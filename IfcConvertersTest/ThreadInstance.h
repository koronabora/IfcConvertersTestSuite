#pragma once

#include <QObject>
#include <QThread>
#include <QPointer>
#include <QApplication>

#include <mutex>

class ThreadInstanceWrapper : public QObject
{
	Q_OBJECT
public:
	ThreadInstanceWrapper(QObject* parent = nullptr): QObject(parent) {};
	virtual ~ThreadInstanceWrapper() { emit unloaded(); };

	virtual void init() {};
signals:
	void unloaded();
};

class ThreadInstance : public QThread
{
	Q_OBJECT

public:
	ThreadInstance(QThread* parent = nullptr)
		: QThread(parent)
		, m_worker(nullptr)
	{
	}

	~ThreadInstance()
	{
	}

	void run() override
	{
		QThread::exec();
	}

	// also waits until thread stops
	void stop()
	{
		if (m_worker)
			delete m_worker;
		wait();
	}

	// also execute thread
	void set(ThreadInstanceWrapper* obj, QThread::Priority priority = QThread::Priority::NormalPriority)
	{
		if (obj)
		{
			m_worker = obj;
			m_worker->moveToThread(this);
			connect(this, &QThread::started, m_worker, &ThreadInstanceWrapper::init);
			connect(m_worker, &ThreadInstanceWrapper::unloaded, this, &ThreadInstance::quit);
		}
		start(priority);
	}

	template<class T>
	T* get()
	{
		T* res = static_cast<T*>(m_worker);
		return res;
	}

	ThreadInstanceWrapper* get()
	{
		return m_worker;
	}

private:
	ThreadInstanceWrapper* m_worker;
};


