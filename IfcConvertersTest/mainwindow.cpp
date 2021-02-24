#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_logField = ui->plainTextEdit;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::log(const QString& str)
{
  if (m_logField)
    m_logField->appendPlainText(str);
}