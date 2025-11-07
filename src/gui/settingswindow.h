#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QDialog>
#include "../core/manager.h"

class Manager;

namespace Ui {
class Settingswindow;
}

class Settingswindow : public QDialog
{
    Q_OBJECT

public:
    explicit Settingswindow(Manager *managerRef, QWidget *parent = nullptr);
    ~Settingswindow();

    void closeEvent(QCloseEvent *event) override;

    void sw_show();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

signals:
    void signal_updateTextMainWindow();

private slots:
    void on_CKBX_OFE_checkStateChanged(const Qt::CheckState &arg1);

    void on_CKBX_DQ_checkStateChanged(const Qt::CheckState &arg1);

    void on_CKBX_EAC_checkStateChanged(const Qt::CheckState &arg1);

    void on_CMBX_LANG_activated(int index);

    void on_BTN_TDF_OPEN_clicked();

    void on_LN_TGEN_INP_textChanged();

    void on_LN_TGEN_INP2_textChanged();

    void on_BTN_RESET_clicked();

    void on_BTN_SAVE_clicked();

    void on_LN_SSID_textChanged(const QString &arg1);

private:
    Ui::Settingswindow *ui;
    Manager *manager;

    Manager::appSettings &settings;
    Manager::appSettings editSettings;

    QPoint m_dragStartPosition;
    bool m_isDragging = false;
    QWidget *titleBar = nullptr;

    void Init();
    void InitTitleBar();
    void InitLang();

    void sw_setData();
    void checkVariables();

    bool settingsChanged = false;
    bool restartRequired = false;
    bool saveButtonUsed = false;

    QMap<QString, QString> params = { // dummy text for preview
        {"user", "LISA"},
        {"caption", "First Emmys experience with my White lotus family 🪷"},
        {"link", "https://www.instagram.com/lalalalisa_m/p/DOqalquE4cU/"}
    };

    QMap<QString, QString> paramsStory = { // dummy text for preview
        {"user", "LLOUD"},
        {"link", "https://www.instagram.com/stories/wearelloud/"}
    };
};

#endif // SETTINGSWINDOW_H
