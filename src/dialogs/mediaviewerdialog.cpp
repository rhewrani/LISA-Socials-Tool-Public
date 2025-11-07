#include "mediaviewerdialog.h"
#include "ui_mediaviewerdialog.h"

MediaViewerDialog::MediaViewerDialog(Manager *managerRef, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MediaViewerDialog)
    , manager(managerRef)
{
    ui->setupUi(this);

    m_videoWidget = new QVideoWidget(this);
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);

    Init();
}

MediaViewerDialog::~MediaViewerDialog()
{
    delete ui;
}

void MediaViewerDialog::displayMediaContent(Instagram::contentChild *child)
{
    if (child->mediaUrl.isEmpty()) return;
    displayChild = *child;

    if (child->type == "Image") {
        ui->BTN_PLBK->setVisible(false);
        ui->BTN_SOUN->setVisible(false);
        ui->WDGT_MDVW_VID->setVisible(false);
        ui->WDGT_MDVW_IMG->setVisible(true);

        m_player->stop();
        int dimX = child->dimensionWidth;
        int dimY = child->dimensionHeight;
        manager->loadPixmap(child->mediaUrl, child->id, dimX, dimY, [this]
                            (const QPixmap &pixmap) {
                                ui->WDGT_MDVW_IMG->setPixmap(pixmap.scaled(350, 500, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                            });
    } else {
        ui->BTN_PLBK->setVisible(true);
        ui->BTN_SOUN->setVisible(true);
        ui->WDGT_MDVW_VID->setVisible(true);
        ui->WDGT_MDVW_IMG->setVisible(false);

        m_player->audioOutput()->setVolume(0.40);
        m_player->audioOutput()->setMuted(false);
        m_player->setSource(child->videoUrl);
        m_player->play();
    }

    ui->LBL_EXPR->setHidden(child->story_expires.isEmpty());
    if (!child->story_expires.isEmpty()) {
        ui->LBL_EXPR->setText(_("NODE_STRY_EXP").arg(child->story_expires));
        resize(370, 680);
        setMinimumHeight(680);
    }

    if (!child->accessabilityCaption.isEmpty()) fitTextToLabel(ui->LBL_ACPT, child->accessabilityCaption);
    ui->LBL_ACPT->setHidden(child->accessabilityCaption.isEmpty());
}

void MediaViewerDialog::displayPfp(Instagram::contentChild child)
{
    displayChild = child;

    setMinimumHeight(400);
    resize(370, 400);
    ui->WDGT_MDVW_IMG->resize(350, 300);
    ui->BTN_DWLD->move(320, 320);

    ui->BTN_PLBK->setVisible(false);
    ui->BTN_SOUN->setVisible(false);
    ui->WDGT_MDVW_VID->setVisible(false);
    ui->WDGT_MDVW_IMG->setVisible(true);

    m_player->stop();

    manager->loadPixmap(child.mediaUrl, child.id, 300, 300, [this]
                        (const QPixmap &pixmap) {
                            ui->WDGT_MDVW_IMG->setPixmap(pixmap.scaled(300, 300, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                        });


}

bool MediaViewerDialog::eventFilter(QObject *obj, QEvent *event)
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

void MediaViewerDialog::Init()
{
    InitTitleBar();
    InitLang();
    ui->LBL_ACPT->setVisible(false);

    if (!ui->WDGT_MDVW_VID->layout()) {
        ui->WDGT_MDVW_VID->setLayout(new QVBoxLayout);
    }
    m_player->setVideoOutput(m_videoWidget);
    ui->WDGT_MDVW_VID->layout()->addWidget(m_videoWidget);
    m_videoWidget->setMinimumSize(338, 500);
    m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_player->setAudioOutput(m_audioOutput);
    m_player->setLoops(QMediaPlayer::Infinite);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::StoppedState || state == QMediaPlayer::PausedState) {
            ui->BTN_PLBK->setIcon(QIcon(":/images/play_white.png"));
        } else if (state == QMediaPlayer::PlayingState) {
            ui->BTN_PLBK->setIcon(QIcon(":/images/pause_white.png"));
        }
    });

    connect(m_audioOutput, &QAudioOutput::mutedChanged, this, [this](bool muted) {
        if (muted) {
            ui->BTN_SOUN->setIcon(QIcon(":/images/volume-off_white.png"));
        } else {
            ui->BTN_SOUN->setIcon(QIcon(":/images/volume-on_white.png"));
        }

    });
}

void MediaViewerDialog::InitTitleBar()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);

    titleBar = new QWidget(this);
    titleBar->setFixedHeight(30);
    titleBar->setStyleSheet("background-color: #1a1a1a; color: white;");

    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(10, 0, 10, 0);

    QLabel *titleLabel = new QLabel(_("MV"));
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

    connect(closeBtn, &QPushButton::clicked, this, &MediaViewerDialog::close);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(ui->mediaviewerwidget);
    setLayout(mainLayout);
}

void MediaViewerDialog::InitLang()
{

}


void MediaViewerDialog::on_BTN_PLBK_clicked()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
    } else {
        m_player->play();
    }
}


void MediaViewerDialog::on_BTN_SOUN_clicked()
{
    if (m_player->audioOutput()->isMuted()) {
        m_player->audioOutput()->setMuted(false);
    } else {
     m_player->audioOutput()->setMuted(true);
    }
}


void MediaViewerDialog::on_BTN_DWLD_clicked()
{
        QMap<int, Instagram::contentChild> map;
        map[0] = displayChild;
        emit signal_downloadMedia(map);

}

