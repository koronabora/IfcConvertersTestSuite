#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include "ThreadInstance.h"

static const QString OUT_DIR = "D:\\IFC_TEST";
static const QString BASE_GLTF_JSON_FILE = "metainfo.json";

//#define USE_QPROCESS

struct ConverterData
{
  QString name;
  QString command;
  bool convertsIfc = false;
  QString outExt = ".glb";
};

static const QVector<ConverterData> CONVERTERS = // ODA have to be first
{
  {
    "file_converter",
    "C:\\Code\\release_21.12\\exe\\vc16_amd64dll\\FileConverter.exe \"%0\" \"%1\" --format=gltf", // out file's name need to have suffix .geometry
    true,
    ".geometry"
  },
  {
    "gltf-pipeline",
    "gltf-pipeline -i \"%0\" -o \"%1\" -d",
     false
  },
  {
    "gltf-transform",
    "gltf-transform draco \"%0\" \"%1\"",
    false
  },
  {
    "IfcConvert",
    "C:\\Code\\release_21.12\\exe\\vc16_amd64dll\\IfcConvert.exe --use-element-guids \"%0\" \"%1\"",
    true
  },
  {
    "gltfpack",
    "gltfpack -c -i \"%0\" -o \"%1\"",
    false
  }
};

class Worker : public ThreadInstanceWrapper
{
  Q_OBJECT

  QString m_workPath;
  QString m_outPath;
  bool m_initOutDir();
  QStringList m_parseBaseGltfFileNames(const QString& dir);
  QMap<QString, QString> m_processedFiles;
  bool m_isAlreadyProcessed(const QString& fileName);

public:
  Worker(const QString& basePath);

public slots:
  void init() override;

signals:
  void log(const QString& str);
};
