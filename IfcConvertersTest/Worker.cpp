#include "Worker.h"
#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QCryptographicHash>

#include <stdlib.h>

Worker::Worker(const QString& basePath)
    :m_workPath(basePath)
{}

bool Worker::m_initOutDir()
{
    QString subFolderName = m_workPath;
    subFolderName.replace("\\", "_").replace("/", "_").replace(":", "_");
    QDir baseDir(OUT_DIR);// +"\\" + subFolderName);
    // check out dir
    if (baseDir.exists(subFolderName))
    {
        baseDir.cd(subFolderName);
        // remove existring dir
        if (baseDir.removeRecursively())
            emit log("Subfolder removed: [" + subFolderName + "]");
    }

    QDir dir;
    m_outPath = OUT_DIR + QDir::separator() + subFolderName;
    if (!dir.mkpath(m_outPath))
    {
        emit log("Could not create subfolder: [" + subFolderName + "]. Aborted.");
        return false;
    }
    return true;
}

QStringList Worker::m_parseBaseGltfFileNames(const QString& dir)
{
    QStringList res;

    const QString fileName = dir + QDir::separator() + BASE_GLTF_JSON_FILE;
    QDirIterator it(dir, { "*.gltf" }, QDir::Filter::Files | QDir::Filter::Readable);
    while (it.hasNext())
        res.append(it.next());
    return res;
}

bool Worker::m_isAlreadyProcessed(const QString& fileName)
{
    QString hash;
    QFile f(fileName);
    if (f.open(QIODevice::OpenModeFlag::ReadOnly))
    {
        hash = QString::fromUtf8(QCryptographicHash::hash(f.readAll(), QCryptographicHash::Algorithm::Keccak_256));
    }
    f.close();
    if (!m_processedFiles.contains(fileName))
    {
        m_processedFiles[fileName] = hash;
        return false;
    }
    if (m_processedFiles[fileName] != hash)
        return false;
    return true;
}

void Worker::init()
{
    QString outPath, outFile, command, logFile;
    QStringList baseGltfFiles;
    size_t cc = 0;
    bool ret;

    emit log("Starting worker with base dir [" + m_workPath + "]");
    if (m_initOutDir() && !CONVERTERS.isEmpty())
    {

        QDirIterator it(m_workPath, { "*.ifc" }, QDir::Filter::AllEntries | QDir::Filter::Readable, QDirIterator::IteratorFlag::Subdirectories | QDirIterator::IteratorFlag::FollowSymlinks);
        while (it.hasNext())
        {
            cc++;
            const QString file = it.next();
            emit log("===============================");
            emit log("Found file #" + QString::number(cc) + ": [" + file + "]");
            emit log("-------------------------------");
            QFileInfo fi(file);
            const QString fileName = fi.baseName(); //

            if (m_isAlreadyProcessed(fileName))
            {
                emit log("Already processed");
                continue;
            }

            outPath = m_outPath + QDir::separator() + QString::number(cc).rightJustified(3, '0') + "." + fileName + QDir::separator();
            // create out dir
            QDir foo;
            foo.mkpath(outPath);
            // copy file into out dir
            QFile::copy(file, outPath + fi.fileName());

            // ODA's CONVERTER CALL
            //outFile = m_outPath + QDir::separator() + fileName + CONVERTERS[0].outExt;
            outFile = m_outPath + QDir::separator() + QString::number(cc).rightJustified(3, '0') + CONVERTERS[0].outExt;
            logFile = outPath + "00" + "_" + fileName + "_" + CONVERTERS[0].name + ".log";
            QDir odaOutDir(outFile);
            command = CONVERTERS[0].command.arg(file).arg(outFile);
#ifdef _DEBUG
            emit log("Converter [" + CONVERTERS[0].name + "] command: " + command);
#endif
#ifdef USE_QPROCESS
            QProcess p0;
            p0.setStandardOutputFile(logFile, QIODevice::OpenModeFlag::Append);
            p0.setStandardErrorFile(logFile, QIODevice::OpenModeFlag::Append);
            p0.setWorkingDirectory(outPath);
            p0.start(command);
            //int ret = _wsystem(command.toStdWString().c_str());
            ret = p0.waitForFinished(1000 * 60 * 60); // 1 hour to work
#else
            command.append(" > \"" + logFile + "\"");
            ret = (0 == _wsystem(command.toStdWString().c_str()));
#endif
            emit log("Converter [" + CONVERTERS[0].name + "] finished: " + (ret ? "OK" : "Error"));
            if (!ret) // call with logging
            {
                command.append(" > \"" + logFile + "\"");
                ret = (0 == _wsystem(command.toStdWString().c_str()));
            }
            if (!ret)
            {
                emit log("Base gltf files not created. Abort.");
                continue; // go to next file
            }
            // read base gltf file
            baseGltfFiles = m_parseBaseGltfFileNames(outFile);
            if (baseGltfFiles.isEmpty())
            {
                emit log("Could not read base gltf files. Abort.");
                continue; // go to next file;
            }
            for (size_t m = 0; m < baseGltfFiles.size(); m++)
            {
                // using gltf extension for copied file.
                const QString newFile = outPath + "0" + QString::number(m) + "_" + fileName + "_" + CONVERTERS[0].name + ".gltf";
                QFile::copy(baseGltfFiles[m], newFile);
                // Also try to copy bin file.
                QFileInfo fooFi(baseGltfFiles[m]);
                if (fooFi.exists())
                {
                    QString oldBinFile = baseGltfFiles[m];
                    oldBinFile.replace(".gltf", ".bin");
                    QString newBinFile = outPath + fooFi.fileName();
                    newBinFile.replace(".gltf", ".bin");
                    QFile::copy(oldBinFile, newBinFile);
                }
                baseGltfFiles[m] = newFile; // now works with new file
            }

            // remove oda's converter out dir
            odaOutDir.removeRecursively();

            // ALL OTHER CONVERTERS TESTS
            for (size_t i = 1; i < CONVERTERS.size(); i++)
            {
                emit log("-------------------------------");
                const auto conv = CONVERTERS[i];
                for (size_t j = 0; j < baseGltfFiles.size(); j++)
                {
                    // run other converter detached
                    QString tempName = outPath + QString::number(i) + QString::number(j) + "_" + fileName + "_" + conv.name;
                    outFile = tempName + conv.outExt;
                    logFile = tempName + ".log";
                    if (conv.convertsIfc)
                        command = conv.command.arg(file).arg(outFile);
                    else
                        command = conv.command.arg(baseGltfFiles[j]).arg(outFile);
#ifdef _DEBUG
                    emit log("Starting converter [" + conv.name + "] using command: " + command);
#endif
#ifdef USE_QPROCESS
                    QProcess* p = new QProcess();
                    connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), p, &QProcess::deleteLater);
                    p->setStandardOutputFile(logFile, QIODevice::OpenModeFlag::Append);
                    p->setStandardErrorFile(logFile, QIODevice::OpenModeFlag::Append);
                    //if (conv.convertsIfc)
                    //  p->startDetached(command);
                    //else
                    //{
                    p->start(command);
                    ret = p0.waitForFinished(1000 * 60 * 60); // 1 hour to work
#else
                    ret = (0 == _wsystem(command.toStdWString().c_str()));
#endif
                    if (!ret) // call with logging
                    {
                        command.append(" > \"" + logFile + "\"");
                        ret = (0 == _wsystem(command.toStdWString().c_str()));
                    }
                    emit log("Converter [" + conv.name + "] finished: " + (ret ? "OK" : "Error"));
                    //}
                }
            }
        }
    }
    emit log("Worker with base dir [" + m_workPath + "] finished");
};