#include "settingswindow.h"
#include "ui_settingswindow.h"

Settingswindow::Settingswindow(Manager *managerRef, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Settingswindow)
    , manager(managerRef)
    , settings(manager->getSettingsStruct())
{
    ui->setupUi(this);
    editSettings = settings;

    Init();
}

Settingswindow::~Settingswindow()
{
    delete ui;
}

void Settingswindow::closeEvent(QCloseEvent *event)
{
    checkVariables();

    if (saveButtonUsed == false && (settingsChanged || restartRequired)) {

        QString str = _("SET_CLOS");
        if (restartRequired) {
            str = _("SET_CLOS2");
        }

        QMessageBox msgBox(this);
        msgBox.setWindowTitle(_("SET_BTN_APP"));
        msgBox.setText(str);
        msgBox.setIcon(QMessageBox::Question);

        QPushButton *saveButton = msgBox.addButton(_("BTN_APP"), QMessageBox::AcceptRole);
        QPushButton *discardButton = msgBox.addButton(_("BTN_DIS"), QMessageBox::DestructiveRole);
        QPushButton *cancelButton = msgBox.addButton(_("BTN_CAN"), QMessageBox::RejectRole);

        msgBox.exec();

        if (msgBox.clickedButton() == saveButton) {

            if (manager->saveSettings(editSettings, restartRequired)) {
                emit signal_updateTextMainWindow();
                event->accept();
            }
        } else if (msgBox.clickedButton() == discardButton) {
            event->accept();  // Close window without saving
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}

void Settingswindow::Init()
{
    ui->LBL_VER->setText(_("SET_VER") + " " + settings.strVersion);
    InitTitleBar();
    InitLang();
}

void Settingswindow::InitTitleBar()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);

    titleBar = new QWidget(this);
    titleBar->setFixedHeight(30);
    titleBar->setStyleSheet("background-color: #1a1a1a; color: white;");

    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(10, 0, 10, 0);

    QLabel *titleLabel = new QLabel(_("SET_TTL"));
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");

    QPushButton *closeBtn = new QPushButton("×");
    closeBtn->setFixedSize(32, 24);
    closeBtn->setStyleSheet(
        "QPushButton { background-color: transparent; color: white; border: none; font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background-color: #e81123; color: white; }"
        "QPushButton:pressed { background-color: #bf0f1d; }"
        );

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(closeBtn);

    titleBar->installEventFilter(this);

    connect(closeBtn, &QPushButton::clicked, this, &Settingswindow::close);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(ui->settingswidget);
    setLayout(mainLayout);
}

void Settingswindow::InitLang()
{
    ui->LBL_TTL->setText(_("SET_TTL"));
    ui->LBL_LANG->setText(_("SET_LANG"));
    ui->LBL_TDF->setText(_("SET_TDF"));
    ui->CKBX_OFE->setText(_("SET_CKBX_OFE"));
    ui->CKBX_DQ->setText(_("SET_CKBX_DQ"));
    ui->CKBX_EAC->setText(_("SET_CKBX_EAC"));

    fitTextToButton(ui->BTN_SAVE, _("SET_BTN_APP"));
    fitTextToButton(ui->BTN_RESET, _("SET_BTN_RES"));
}

void Settingswindow::sw_show()
{
    sw_setData();
    show();
}

bool Settingswindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == titleBar) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                m_isDragging = true;
                m_dragStartPosition = me->globalPos() - frameGeometry().topLeft();
                return true;
            }
        } else if (event->type() == QEvent::MouseMove) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if (m_isDragging && (me->buttons() & Qt::LeftButton)) {
                move(me->globalPos() - m_dragStartPosition);
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            m_isDragging = false;
        }
    }
    return QDialog::eventFilter(obj, event);
}

void Settingswindow::sw_setData()
{
    editSettings = settings;

    ui->CMBX_LANG->setCurrentIndex(editSettings.intLanguage);
    ui->CKBX_DQ->setChecked(editSettings.bEnableDiscordQuoting);
    ui->CKBX_EAC->setChecked(editSettings.bAutoCopyText);
    ui->CKBX_OFE->setChecked(editSettings.bOpenFileExplorerOnSave);
    ui->LN_TDF->setText(editSettings.strDownloadDir);
    ui->LN_SSID->setText(editSettings.strSessionid);
    ui->LN_TGEN_INP->setText(editSettings.presets["instagram_post"]);
    ui->LN_TGEN_INP2->setText(editSettings.presets["instagram_story"]);


    settingsChanged = false;
    restartRequired = false;
    saveButtonUsed = false;
}

void Settingswindow::checkVariables()
{
    restartRequired = false;
    settingsChanged = false;

    if (editSettings.intLanguage != settings.intLanguage) {
        restartRequired = true;
        return;
    }

    if (editSettings.strSessionid != settings.strSessionid) {
        restartRequired = true;
        return;
    }

    if (editSettings.bEnableDiscordQuoting != settings.bEnableDiscordQuoting) {
        settingsChanged = true;
    }

    if (editSettings.bAutoCopyText != settings.bAutoCopyText) {
        settingsChanged = true;
    }

    if (editSettings.bOpenFileExplorerOnSave != settings.bOpenFileExplorerOnSave) {
        settingsChanged = true;
    }

    if (editSettings.strDownloadDir != settings.strDownloadDir) {
        settingsChanged = true;
    }


    if (editSettings.presets["instagram_post"] != settings.presets["instagram_post"]) {
        settingsChanged = true;
    }

    if (editSettings.presets["instagram_story"] != settings.presets["instagram_story"]) {
        settingsChanged = true;
    }
}

void Settingswindow::on_CKBX_OFE_checkStateChanged(const Qt::CheckState &arg1)
{
    editSettings.bOpenFileExplorerOnSave = arg1;
}


void Settingswindow::on_CKBX_DQ_checkStateChanged(const Qt::CheckState &arg1)
{
    editSettings.bEnableDiscordQuoting = arg1;
    manager->generateCopyPasteTextString(ui->LN_TGEN_INP->toPlainText(), ui->LN_TGEN_PRVW, params, editSettings.bEnableDiscordQuoting);
    manager->generateCopyPasteTextString(ui->LN_TGEN_INP2->toPlainText(), ui->LN_TGEN_PRVW2, paramsStory, editSettings.bEnableDiscordQuoting);
}


void Settingswindow::on_CKBX_EAC_checkStateChanged(const Qt::CheckState &arg1)
{
    editSettings.bAutoCopyText = arg1;
}


void Settingswindow::on_CMBX_LANG_activated(int index)
{
    editSettings.intLanguage = index;
}


void Settingswindow::on_BTN_TDF_OPEN_clicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        _("SET_TDF"),
        QDir::currentPath()
        );

    if (!dirPath.isEmpty()) dirPath = dirPath + "/";
    ui->LN_TDF->setText(dirPath);
    editSettings.strDownloadDir = dirPath;
}


void Settingswindow::on_LN_TGEN_INP_textChanged()
{
    editSettings.presets["instagram_post"] = ui->LN_TGEN_INP->toPlainText();
    manager->generateCopyPasteTextString(ui->LN_TGEN_INP->toPlainText(), ui->LN_TGEN_PRVW, params, editSettings.bEnableDiscordQuoting);
}

void Settingswindow::on_LN_TGEN_INP2_textChanged()
{
    editSettings.presets["instagram_story"] = ui->LN_TGEN_INP2->toPlainText();
    manager->generateCopyPasteTextString(ui->LN_TGEN_INP2->toPlainText(), ui->LN_TGEN_PRVW2, paramsStory, editSettings.bEnableDiscordQuoting);
}


void Settingswindow::on_BTN_RESET_clicked()
{
    editSettings.presets["instagram_post"] = "{user} shared a post on Instagram!\n{caption}\n\n{link}";
    editSettings.presets["instagram_story"] = "{user} shared a story on Instagram!\n\n{link}";
    editSettings.intLanguage = 0;
    editSettings.bAutoCopyText = true;
    editSettings.bEnableDiscordQuoting = true;
    editSettings.bOpenFileExplorerOnSave = true;
    editSettings.strDownloadDir = "";
    editSettings.strSessionid = "";

    ui->CMBX_LANG->setCurrentIndex(editSettings.intLanguage);
    ui->CKBX_DQ->setChecked(editSettings.bEnableDiscordQuoting);
    ui->CKBX_EAC->setChecked(editSettings.bAutoCopyText);
    ui->CKBX_OFE->setChecked(editSettings.bOpenFileExplorerOnSave);
    ui->LN_TDF->setText(editSettings.strDownloadDir);
    ui->LN_SSID->setText(editSettings.strSessionid);
    ui->LN_TGEN_INP->setText(editSettings.presets["instagram_post"]);
    ui->LN_TGEN_INP2->setText(editSettings.presets["instagram_story"]);

}


void Settingswindow::on_BTN_SAVE_clicked()
{
    checkVariables();

    if (restartRequired) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(_("SET_BTN_APP"));
        msgBox.setText(_("SET_RSTT"));
        msgBox.setIcon(QMessageBox::Question);

        QPushButton *yesButton = msgBox.addButton(_("YES"), QMessageBox::YesRole);
        QPushButton *noButton = msgBox.addButton(_("NO"), QMessageBox::NoRole);

        msgBox.exec();

        if (msgBox.clickedButton() == noButton) {
            return;
        }
    }

    saveButtonUsed = true;

    if (manager->saveSettings(editSettings, restartRequired)) {
        emit signal_updateTextMainWindow();
        close();
    }
}


void Settingswindow::on_LN_SSID_textChanged(const QString &arg1)
{
    editSettings.strSessionid = arg1;
}

