#include "ui_euLog.h"
#include "LogCollectorModel.hh"
#include "LogDialog.hh"
#include "eudaq/LogCollector.hh"
#include "eudaq/Logger.hh"
#include <QMainWindow>
#include <QMessageBox>
#include <QItemDelegate>
#include <QPainter>

#include <iostream>
#include <QSettings>

// To make Qt behave on OSX (to be checked on other OSes)
#define MAGIC_NUMBER 22

class LogItemDelegate : public QItemDelegate {
public:
  LogItemDelegate(LogCollectorModel *model);

private:
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const;
  LogCollectorModel *m_model;
};

class LogCollectorGUI : public QMainWindow,
                        public Ui::wndLog,
                        public eudaq::LogCollector {
  Q_OBJECT
public:
  LogCollectorGUI(const std::string &runcontrol,
                  const std::string &listenaddress,
		  const std::string &directory,
		  int loglevel)
      : QMainWindow(0, 0),
        eudaq::LogCollector(runcontrol, listenaddress, directory), m_delegate(&m_model) {
    setupUi(this);
    std::string filename;
    viewLog->setModel(&m_model);
    viewLog->setItemDelegate(&m_delegate);
    for (int i = 0; i < LogMessage::NumColumns(); ++i) {
      int w = LogMessage::ColumnWidth(i);
      if (w >= 0)
        viewLog->setColumnWidth(i, w);
    }
    int level = 0;
    for (;;) {
      std::string text = eudaq::Status::Level2String(level);
      if (text == "")
        break;
      text = eudaq::to_string(level) + "-" + text;
      cmbLevel->addItem(text.c_str());
      level++;
    }
    cmbLevel->setCurrentIndex(loglevel);
    QRect geom(-1,-1, 100, 100);
    QRect geom_from_last_program_run;
    QSettings settings("EUDAQ collaboration", "EUDAQ");

    settings.beginGroup("MainWindowEuLog");
    geom_from_last_program_run.setSize(settings.value("size", geom.size()).toSize());
    geom_from_last_program_run.moveTo(settings.value("pos", geom.topLeft()).toPoint());
    settings.endGroup();

    QSize fsize = frameGeometry().size();
  if ((geom.x() == -1)||(geom.y() == -1)||(geom.width() == -1)||(geom.height() == -1)) {
    if ((geom_from_last_program_run.x() == -1)||(geom_from_last_program_run.y() == -1)||(geom_from_last_program_run.width() == -1)||(geom_from_last_program_run.height() == -1)) {
      geom.setX(x()); 
      geom.setY(y());
      geom.setWidth(fsize.width());
      geom.setHeight(fsize.height());
      move(geom.topLeft());
      resize(geom.size());
    } else {
      move(geom_from_last_program_run.topLeft());
      resize(geom_from_last_program_run.size());
    }
  }
    connect(this, SIGNAL(RecMessage(const eudaq::LogMessage &)), this,
            SLOT(AddMessage(const eudaq::LogMessage &)));
    try {
      if (filename != "")
        LoadFile(filename);
    } catch (const std::runtime_error &) {
      // probably file not found: ignore
    }
    setWindowIcon(QIcon("../images/Icon_euLog.png"));
  }

  ~LogCollectorGUI(){
    QSettings settings("EUDAQ collaboration", "EUDAQ");
    settings.beginGroup("MainWindowEuLog");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.endGroup();
  }

protected:
  void LoadFile(const std::string &filename) {
    std::vector<std::string> sources = m_model.LoadFile(filename);
    std::cout << "File loaded, sources = " << sources.size() << std::endl;
    for (size_t i = 0; i < sources.size(); ++i) {
      size_t dot = sources[i].find('.');
      if (dot != std::string::npos) {
        AddSender(sources[i].substr(0, dot), sources[i].substr(dot + 1));
      } else {
        AddSender(sources[i]);
      }
    }
  }
  void DoConnect(std::shared_ptr<const eudaq::ConnectionInfo> id) override{
    eudaq::mSleep(100);
    CheckRegistered();
    EUDAQ_INFO("Connection from " + to_string(id));
    AddSender(id->GetType(), id->GetName());
  }
  void DoDisconnect(std::shared_ptr<const eudaq::ConnectionInfo> id) override{
    EUDAQ_INFO("Disconnected " + to_string(*id));
  }
  void DoReceive(const eudaq::LogMessage &msg) override{
    CheckRegistered();
    emit RecMessage(msg);
  }
  void DoTerminate() override{
    std::cout << "terminating!" << std::endl;
    QApplication::quit();
  }
  void AddSender(const std::string &type, const std::string &name = "");
  void closeEvent(QCloseEvent *) {
    std::cout << "Closing!" << std::endl;
    QApplication::quit();
  }
signals:
  void RecMessage(const eudaq::LogMessage &msg);
private slots:
  void on_cmbLevel_currentIndexChanged(int index) {
    m_model.SetDisplayLevel(index);
  }
  void on_cmbFrom_currentIndexChanged(const QString &text) {
    std::string type = text.toStdString(), name;
    size_t dot = type.find('.');
    if (dot != std::string::npos) {
      name = type.substr(dot + 1);
      type = type.substr(0, dot);
    }
    m_model.SetDisplayNames(type, name);
  }
  void on_txtSearch_editingFinished() {
    m_model.SetSearch(txtSearch->displayText().toStdString());
  }
  void on_viewLog_activated(const QModelIndex &i) {
    new LogDialog(m_model.GetMessage(i.row()));
  }
  void AddMessage(const eudaq::LogMessage &msg) {
    QModelIndex pos = m_model.AddMessage(msg);
    if (pos.isValid())
      viewLog->scrollTo(pos);
  }

private:
  static void CheckRegistered() {
    static bool registered = false;
    if (!registered) {
      qRegisterMetaType<QModelIndex>("QModelIndex");
      qRegisterMetaType<eudaq::LogMessage>("eudaq::LogMessage");
      registered = true;
    }
  }
  LogCollectorModel m_model;
  LogItemDelegate m_delegate;
};
