#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    manager = new Manager(this, this);

    settingswindow = new Settingswindow(manager, this);

    infodialog = new Infodialog(manager, this);

    model = new FeedListModel(this);
    cModel = new ChildMediaModel(this);

    m_videoWidget = new QVideoWidget(this);
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);

    clipboard = QGuiApplication::clipboard();

    Logger::instance()->setParentWidget(this, 0);

    Init();

}

void MainWindow::updateProfileInfoUI(Instagram::userData* user, bool loadStory)
{
    if (!user->allowUpdateProfileInfoUI) return;

    if (user->username == "lalalalisa_m") {
        setPfpImageFromURL(ui->INST_LISA_PFP, user->profilePicUrl, user->id, 150, 150);
        ui->INST_LISA_USER->setText(user->username);
        ui->INST_LISA_NAME->setText(user->fullname);
        ui->INST_LISA_BIO->setText(user->biography);
        ui->INST_LISA_FOL_CON->setText(formatNumber(user->followersCount));
        ui->INST_LISA_POST_CON->setText(formatNumber(user->postsCount));
    } else if (user->username == "wearelloud") {
        setPfpImageFromURL(ui->INST_LOUD_PFP, user->profilePicUrl, user->id, 150, 150);
        ui->INST_LOUD_USER->setText(user->username);
        ui->INST_LOUD_NAME->setText(user->fullname);
        ui->INST_LOUD_BIO->setText(user->biography);
        ui->INST_LOUD_FOL_CON->setText(formatNumber(user->followersCount));
        ui->INST_LOUD_POST_CON->setText(formatNumber(user->postsCount));
    } else {
        setPfpImageFromURL(ui->INST_LFAM_PFP, user->profilePicUrl, user->id, 150, 150);
        ui->INST_LFAM_USER->setText(user->username);
        ui->INST_LFAM_NAME->setText(user->fullname);
        ui->INST_LFAM_BIO->setText(user->biography);
        ui->INST_LFAM_FOL_CON->setText(formatNumber(user->followersCount));
        ui->INST_LFAM_POST_CON->setText(formatNumber(user->postsCount));
    }

    if (loadStory) {
        manager->instagram_GET_Story(user->username);
    }

    user->allowUpdateProfileInfoUI = false;
}


void MainWindow::updateProfileFeedUI(Instagram::userData *user)
{
    if (!user->allowUpdateProfileFeedUI) return;

    if (user->shouldFeedUIRefresh) {
        model->setFeed(user->feed);
        downloadImagesForFeed(user->feed.values(), 0);
    } else {
        if (!user->appendFeed.isEmpty()) {
            model->appendPosts(user->appendFeed);
            int startRow = model->rowCount() - user->appendFeed.size();
            downloadImagesForFeed(user->appendFeed, startRow);
            ui->INST_LV_FEED->scrollToBottom();
        }
    }

    user->appendFeed.clear();
    user->allowUpdateProfileFeedUI = false;
    user->shouldFeedUIRefresh = false;
    ui->INST_WDGT_USER->setCurrentIndex(manager->instagram_getCurrentSelectedUser());

    show();
    raise();
    activateWindow();
    if (overlay) {
        overlay->hide();
        overlay->deleteLater();
        overlay = nullptr;
    }
    initialLoad();
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
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
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::Init()
{
    InitUI();

    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::StoppedState || state == QMediaPlayer::PausedState) {
            ui->INST_BTN_PLBK->setIcon(QIcon(":/images/play_white.png"));
        } else if (state == QMediaPlayer::PlayingState) {
            ui->INST_BTN_PLBK->setIcon(QIcon(":/images/pause_white.png"));
        }
    });
    connect(m_audioOutput, &QAudioOutput::mutedChanged, this, [this](bool muted) {
        if (muted) {
            ui->INST_BTN_SOUN->setIcon(QIcon(":/images/volume-off_white.png"));
        } else {
            ui->INST_BTN_SOUN->setIcon(QIcon(":/images/volume-on_white.png"));
        }

    });

    connect(settingswindow, &Settingswindow::signal_updateTextMainWindow, this, &MainWindow::updateGeneratedText);

    connect(INST_MDCT_PFP, &ClickableLabel::clicked, this, &MainWindow::openPfpViewer);

    connect(INST_MDIA_PFP, &ClickableLabel::clicked, this, &MainWindow::openPfpViewer);

}

void MainWindow::InitTitleBar()
{
    setWindowFlags(Qt::FramelessWindowHint);

    titleBar = new QWidget(this);
    titleBar->setFixedHeight(30);
    titleBar->setStyleSheet("background-color: #1a1a1a; color: white;");

    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(10, 0, 10, 0);

    QLabel *titleLabel = new QLabel("LISA Socials Tool");
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");

    QPushButton *minimizeBtn = new QPushButton("–");
    minimizeBtn->setFixedSize(32, 24);
    minimizeBtn->setStyleSheet(
        "QPushButton { background-color: transparent; color: white; border: none; font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background-color: #3a3a3a; }"
        "QPushButton:pressed { background-color: #2a2a2a; }"
        );

    QPushButton *closeBtn = new QPushButton("×");
    closeBtn->setFixedSize(32, 24);
    closeBtn->setStyleSheet(
        "QPushButton { background-color: transparent; color: white; border: none; font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background-color: #e81123; color: white; }"
        "QPushButton:pressed { background-color: #bf0f1d; }"
        );

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(minimizeBtn);
    titleLayout->addWidget(closeBtn);

    titleBar->installEventFilter(this);

    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::close);

    QMenuBar* embeddedMenuBar = ui->menubar;
    embeddedMenuBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(embeddedMenuBar);
    mainLayout->addWidget(ui->centralwidget);

    QWidget *central = new QWidget(this);
    central->setLayout(mainLayout);
    setCentralWidget(central);
}

void MainWindow::InitUI()
{

    InitTitleBar();

    INST_MDCT_PFP = new ClickableLabel(ui->INST_PG_MDCT);
    INST_MDCT_PFP->setObjectName("INST_MDCT_PFP");
    INST_MDCT_PFP->setGeometry(QRect(10, 370, 50, 50));
    sizePolicy().setHeightForWidth(INST_MDCT_PFP->sizePolicy().hasHeightForWidth());
    INST_MDCT_PFP->setSizePolicy(sizePolicy());
    INST_MDCT_PFP->setStyleSheet(QString::fromUtf8("border-radius: 25px;\n"
                                                   "border: 2px solid #0c0c0c;\n"
                                                   "background-color: lightgray; \n"
                                                   "background-position: center;\n"
                                                   "background-repeat: no-repeat;\n"
                                                   "background-image: url(:/images/lisa.jpg);\n"
                                                   ""));
    INST_MDCT_PFP->setScaledContents(true);

    INST_MDIA_PFP = new ClickableLabel(ui->INST_PG_MDIA);
    INST_MDIA_PFP->setObjectName("INST_MDIA_PFP");
    INST_MDIA_PFP->setGeometry(QRect(370, 40, 50, 50));
    sizePolicy().setHeightForWidth(INST_MDIA_PFP->sizePolicy().hasHeightForWidth());
    INST_MDIA_PFP->setSizePolicy(sizePolicy());
    INST_MDIA_PFP->setStyleSheet(QString::fromUtf8("border-radius: 25px;\n"
                                                   "border: 2px solid #0c0c0c;\n"
                                                   "background-color: lightgray; \n"
                                                   "background-position: center;\n"
                                                   "background-repeat: no-repeat;\n"
                                                   "background-image: url(:/images/lisa.jpg);\n"
                                                   ""));
    INST_MDIA_PFP->setScaledContents(true);

    setLabelTextWithEmoji(ui->INST_LBL_MDST, _("NO_MDIA"), "lisa-think.png");
    ui->INST_WDGT_PRVW_PGS->setVisible(false);
    ui->INST_MDIA_VERI->setVisible(false);
    ui->INST_MDIA_ACPT->setVisible(false);
    ui->INST_MDIA_VIEW_ICON->setVisible(false);
    ui->INST_MDIA_VIEW->setVisible(false);
    ui->INST_MDIA_CAPT->setFixedSize(280, 190);
    ui->INST_MDIA_NEW->setVisible(false);
    ui->INST_MDCT_NEW->setVisible(false);
    ui->INST_LISA_STRY->setVisible(false);
    ui->INST_LOUD_STRY->setVisible(false);
    ui->INST_LFAM_STRY->setVisible(false);
    ui->LBL_LISA->setVisible(false);
    ui->INST_LN_RES->setContextMenuPolicy(Qt::DefaultContextMenu);

    auto feed = ui->INST_LV_FEED;
    feed->setModel(model);
    feed->setItemDelegate(new FeedItemDelegate(feed));
    feed->setResizeMode(QListView::Adjust);
    feed->setGridSize(QSize(170, 230));
    feed->setIconSize(QSize(170, 230));
    feed->setSpacing(0);

    auto postChildren = ui->INST_LV_POST;
    postChildren->setModel(cModel);
    postChildren->setViewMode(QListView::IconMode);
    postChildren->setGridSize(QSize(170, 230));
    postChildren->setIconSize(QSize(170, 230));
    postChildren->setSpacing(2);
    postChildren->setItemDelegate(new VideoOverlayDelegate(this));

    if (!ui->INST_WDGT_VID->layout()) {
        ui->INST_WDGT_VID->setLayout(new QVBoxLayout);
    }
    m_player->setVideoOutput(m_videoWidget);
    ui->INST_WDGT_VID->layout()->addWidget(m_videoWidget);
    m_videoWidget->setMinimumSize(335, 440);
    m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_player->setAudioOutput(m_audioOutput);
    m_player->setLoops(QMediaPlayer::Infinite);

    InitLang();
}

void MainWindow::InitLang()
{
    fitTextToLabel(ui->INST_LISA_FOL, _("USER_FOL"));
    fitTextToLabel(ui->INST_LISA_POST, _("USER_POST"));
    fitTextToLabel(ui->INST_LOUD_FOL, _("USER_FOL"));
    fitTextToLabel(ui->INST_LOUD_POST, _("USER_POST"));
    fitTextToLabel(ui->INST_LFAM_FOL, _("USER_FOL"));
    fitTextToLabel(ui->INST_LFAM_POST, _("USER_POST"));

    ui->INST_LBL_OR->setText(_("OR"));
    ui->INST_LN_LINK->setPlaceholderText(_("LINK"));

    fitTextToButton(ui->INST_BTN_DOWN, _("BTN_DOWN"));
    fitTextToButton(ui->BTN_MGC, _("BTN_MGC"));
    fitTextToButton(ui->BTN_SAVE, _("BTN_SELC_DOWN"));
    fitTextToButton(ui->INST_BTN_SELC, _("BTN_SELC_ALL"));
    fitTextToButton(ui->INST_BTN_DSLC, _("BTN_SELC_NONE"));

    ui->menuMenu->setTitle(_("MBR_MENU"));
    ui->MENU_OPEN_SETT->setText(_("MBR_MENU_SET"));
    ui->MENU_OPEN_INFO->setText(_("MBR_MENU_INFO"));
}

void MainWindow::setPfpImageFromURL(QLabel *target, QString &url, QString &id, int dimX, int dimY)
{

    manager->loadPixmap(url, id, dimX, dimY,
                        [this, id, target, dimX](const QPixmap &pixmap) {
        QPixmap rounded(pixmap.size());
        rounded.fill(Qt::transparent);

        QPainter painter(&rounded);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addEllipse(pixmap.rect());
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, pixmap);

        target->setPixmap(rounded);
        target->setScaledContents(true);
        target->setStyleSheet(QString(
                                  "border-radius: %1px;"
                                  "border: 2px solid #0c0c0c;"
                                  "background-color: lightgray;"
                                  "background-position: center;"
                                  "background-repeat: no-repeat;"
                                  ).arg(dimX/2));
    });


}

void MainWindow::toggleStoryButton(const QString &username)
{
    if (username == "lalalalisa_m") {
            ui->INST_LISA_STRY->setVisible(true);
    } else if (username == "wearelloud") {
            ui->INST_LOUD_STRY->setVisible(true);
    } else {
            ui->INST_LFAM_STRY->setVisible(true);
    }
}

void MainWindow::downloadImagesForFeed(const QList<Instagram::contentNode> &posts, int startRow)
{
    for (int i = 0; i < posts.size(); ++i) {
        int row = startRow + i;
        if (!model->hasPixmapForRow(row)) {
            int dimX = posts[i].originalDimensionWidth;
            int dimY = posts[i].originalDimensionHeight;
            manager->loadPixmap(posts[i].imageUrl, posts[i].id, dimX, dimY,
                                [this, row](const QPixmap &pixmap) {
                                    model->setPixmapForRow(row, pixmap);
                                });
        }
    }
}

void MainWindow::downloadChildMediaImages(const QMap<int, Instagram::contentChild> &children)
{
    auto childModel = qobject_cast<ChildMediaModel*>(ui->INST_LV_POST->model());
    if (!childModel) return;

    auto childList = children.values();
    for (int i = 0; i < children.size(); ++i) {
        const QString &url = children[i].mediaUrl;
        if (!childModel->hasPixmapForRow(i)) {
            int dimX = childList[i].dimensionWidth;
            int dimY = childList[i].dimensionHeight;
            manager->loadPixmap(url, childList[i].id, dimX, dimY,
                                [childModel, i](const QPixmap &pixmap) {
                                    childModel->setPixmapForRow(i, pixmap);
                                });
        }
    }
}

void MainWindow::queueSaveMedia(const QMap<int, Instagram::contentChild> &map)
{

    QString failed;
    QString username = (!currentSelectedNode->foreignOwnerUsername.isEmpty()) ? currentSelectedNode->foreignOwnerUsername : manager->currentUser->username;
    QString id = (currentSelectedNode->type == "Story") ? "Stories" : currentSelectedNode->shortcode;

    auto &settings = manager->getSettingsStruct();

    QString baseDir = QCoreApplication::applicationDirPath();
    QString pathFromBaseDir = QString("/downloads/%1/%2/").arg(username, id);
    QString openDir = QString("%1/%2/").arg(baseDir, pathFromBaseDir);
    QString downloadPath = pathFromBaseDir;
    if (!settings.strDownloadDir.isEmpty()) {
        downloadPath = settings.strDownloadDir + QString("%1/%2/").arg(username, id);
        openDir = downloadPath;
    }


        if (!map.isEmpty() || currentSelectedNode->type == "MediaDict" || currentSelectedNode->type == "Story") {
            bool res = true;
            auto &children = map.isEmpty() ? currentSelectedNode->children : map;
            for (int i = 0; i < children.size(); ++i) {
                QString useFileName = (currentSelectedNode->type == "Story") ? children[i].id : QString::number(children[i].childIndex);
                if (children[i].type == "Image") {
                    QString imagePath = QString("%1%2.jpg").arg(downloadPath, useFileName);
                    manager->loadPixmap(children[i].mediaUrl, children[i].id, children[i].dimensionWidth, children[i].dimensionHeight, [this, &res, imagePath]
                            (const QPixmap &pixmap) {
                            res = manager->saveMedia(pixmap, imagePath);
                    });
                } else if (children[i].type == "Video") {
                    QString videoPath = QString("%1%2.mp4").arg(downloadPath, useFileName);
                    bool res = manager->saveMediaVideo(children[i].videoUrl, videoPath);
                    if (!res) failed = id;
                } else {
                    QString relPath = QString("/downloads/%1/").arg(username);
                    openDir = QString("%1/%2").arg(baseDir, relPath);
                    if (!settings.strDownloadDir.isEmpty()) {
                        relPath = QString("%1/%2/").arg(settings.strDownloadDir, username);
                        openDir = relPath;
                    }
                    QString pfpPath = QString("%1%2.jpg").arg(relPath, children[i].id);

                    manager->loadPixmap(children[i].mediaUrl, children[i].id, 300, 300, [this, &res, pfpPath]
                                        (const QPixmap &pixmap) {
                        res = manager->saveMedia(pixmap, pfpPath);
                                        });
                }
            }
        } else if (currentSelectedNode->type == "Video") {
            QString videoPath = QString("%1%2.mp4").arg(downloadPath, currentSelectedNode->id);
                bool res = manager->saveMediaVideo(currentSelectedNode->videoUrl, videoPath);
                if (!res) failed = id;

        } else {
                QString imagePath = QString("%1%2.jpg").arg(downloadPath, currentSelectedNode->id);
                bool res = true;
                manager->loadPixmap(currentSelectedNode->imageUrl, currentSelectedNode->id, currentSelectedNode->originalDimensionWidth, currentSelectedNode->originalDimensionHeight, [this, &res, imagePath]
                                    (const QPixmap pixmap) {
                                        res = manager->saveMedia(pixmap, imagePath);
                                    });
                if (!res) failed = id;
        }

    if (failed.isEmpty() && settings.bOpenFileExplorerOnSave) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(openDir));
    }
}

void MainWindow::openPfpViewer()
{
    if (!currentSelectedNode) {
        return;
    }

    MediaViewerDialog *viewer = new MediaViewerDialog(manager, this);
    Instagram::contentChild child = {
        .type = "Pfp",
        .mediaUrl = currentSelectedNode->foreignOwnerPfpUrl.isEmpty() ? manager->currentUser->profilePicUrl : currentSelectedNode->foreignOwnerPfpUrl,
        .id = currentSelectedNode->foreignOwnerId + "_pfp"
    };

    connect(viewer, &MediaViewerDialog::signal_downloadMedia,
            this, &MainWindow::queueSaveMedia);

    viewer->setAttribute(Qt::WA_DeleteOnClose);
    viewer->displayPfp(child);
    viewer->show();
}


void MainWindow::displayNodeContent(Instagram::contentNode *node)
{
    currentSelectedNode = node;
    QMap<QString, QString> params;

    if (node->type == "MediaDict" || node->type == "Story") {
        m_player->stop();

        auto childModel = qobject_cast<ChildMediaModel*>(ui->INST_LV_POST->model());
        if (childModel) {
            childModel->setChildren(node->children);
            downloadChildMediaImages(node->children);
        }

        Instagram::userData* user = manager->currentUser;
        if (!node->foreignOwnerUsername.isEmpty()) {
            params["user"] = node->foreignOwnerFullname;
            setPfpImageFromURL(INST_MDCT_PFP, node->foreignOwnerPfpUrl, node->foreignOwnerId, 50, 50);
            ui->INST_MDCT_USER->setText(node->foreignOwnerUsername);
            setPixmapToText(ui->INST_MDCT_USER, ui->INST_MDCT_VERI, Right, 5);
            ui->INST_MDCT_VERI->setVisible(node->foreignOwnerIsVerified);
        } else {
            params["user"] = manager->currentUser->fullname;
            setPfpImageFromURL(INST_MDCT_PFP, manager->currentUser->profilePicUrl, manager->currentUser->id, 50, 50);
            ui->INST_MDCT_USER->setText(user->username);
            setPixmapToText(ui->INST_MDCT_USER, ui->INST_MDCT_VERI, Right, 5);
            ui->INST_MDCT_VERI->setVisible(true);
        }

        if (node->type == "Story") {
            ui->INST_MDCT_COM->setHidden(true);
            ui->INST_MDCT_COM_PIC->setHidden(true);
            ui->INST_MDCT_LIKE->setHidden(true);
            ui->INST_MDCT_LIKE_PIC->setHidden(true);
        }

        if (!node->caption.isEmpty()) {
            ui->INST_MDCT_CAPT->setText(node->caption);
        }
        ui->INST_MDCT_CAPT->setHidden(node->caption.isEmpty());

        if (!node->location.isEmpty()) {
            ui->INST_MDCT_LOC->setText(node->location);
        }
        ui->INST_MDCT_LOC->setHidden(node->location.isEmpty());

        if (!node->timestamp.isEmpty()) {
            fitTextToLabel(ui->INST_MDCT_TMSP, _("NODE_TMSP") + node->timestamp);
        }
        ui->INST_MDCT_TMSP->setHidden(node->timestamp.isEmpty());
        ui->INST_MDCT_NEW->setVisible(node->isNew);

        if (node->commentCount == -1) {
            ui->INST_MDCT_COM->setToolTip(_("NODE_COM"));
            fitTextToLabel(ui->INST_MDCT_COM, _("NODE_COM2"));
            ui->INST_MDCT_COM->setStyleSheet("color: #a8a8a8; font-size: 10pt;");
        } else {
            ui->INST_MDCT_COM->setText(formatNumber(node->commentCount));
            ui->INST_MDCT_COM->setToolTip("");
            ui->INST_MDCT_COM->setStyleSheet("color: white; font-size: 12pt;");
        }

        if (node->likeCount == -1) {
            ui->INST_MDCT_LIKE->setToolTip(_("NODE_LIKE"));
            ui->INST_MDCT_LIKE->setStyleSheet("color: #a8a8a8; font-size: 10pt;");
            fitTextToLabel(ui->INST_MDCT_COM, _("NODE_LIKE2"));
        } else {
            ui->INST_MDCT_LIKE->setText(formatNumber(node->likeCount));
            ui->INST_MDCT_COM->setToolTip("");
            ui->INST_MDCT_LIKE->setStyleSheet("color: white; font-size: 12pt;");
        }

        ui->INST_WDGT_PRVW_PGS->setCurrentIndex(0);

    } else {

        if (node->type == "Image") {
            ui->INST_WDGT_VID->setVisible(false);
            ui->INST_BTN_PLBK->setVisible(false);
            ui->INST_BTN_SOUN->setVisible(false);
            ui->INST_MDIA_IMG->setVisible(true);
            m_player->stop();
            int dimX = node->originalDimensionWidth;
            int dimY = node->originalDimensionHeight;
            manager->loadPixmap(node->imageUrl, node->id, dimX, dimY, [this]
                                (const QPixmap &pixmap) {
                                    ui->INST_MDIA_IMG->setPixmap(pixmap.scaled(335, 440, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                                });
        } else {
            ui->INST_WDGT_VID->setVisible(true);
            ui->INST_BTN_PLBK->setVisible(true);
            ui->INST_MDIA_IMG->setVisible(false);

            m_player->audioOutput()->setVolume(0.40);
            m_player->setSource(node->videoUrl);
            m_player->play();


        }

        Instagram::userData* user = manager->currentUser;
        if (!node->foreignOwnerUsername.isEmpty()) {
            params["user"] = node->foreignOwnerFullname;
            setPfpImageFromURL(INST_MDIA_PFP, node->foreignOwnerPfpUrl, node->foreignOwnerId, 50, 50);
            ui->INST_MDIA_USER->setText(node->foreignOwnerUsername);
            setPixmapToText(ui->INST_MDIA_USER, ui->INST_MDIA_VERI, Right, 5);
            ui->INST_MDIA_VERI->setVisible(node->foreignOwnerIsVerified);
        } else {
            params["user"] = manager->currentUser->fullname;
            setPfpImageFromURL(INST_MDIA_PFP, manager->currentUser->profilePicUrl, manager->currentUser->id, 50, 50);
            ui->INST_MDIA_USER->setText(user->username);
            setPixmapToText(ui->INST_MDIA_USER, ui->INST_MDIA_VERI, Right, 5);
            ui->INST_MDIA_VERI->setVisible(true);
        }

        ui->INST_MDIA_CAPT->setText(node->caption);
            if (!node->accessabilityCaption.isEmpty()) {
            fitTextToLabel(ui->INST_MDIA_ACPT, node->accessabilityCaption);
        }
        ui->INST_MDIA_ACPT->setHidden(node->accessabilityCaption.isEmpty());

        if (!node->location.isEmpty()) {
            ui->INST_MDIA_LOC->setText(node->location);
        }
        ui->INST_MDIA_LOC->setHidden(node->location.isEmpty());

        if (!node->timestamp.isEmpty()) {
            fitTextToLabel(ui->INST_MDIA_TMSP, _("NODE_TMSP") + node->timestamp);
        }
        ui->INST_MDIA_TMSP->setHidden(node->timestamp.isEmpty());
        ui->INST_MDIA_NEW->setVisible(node->isNew);

        if (node->videoViewCount != 0) {
            ui->INST_MDIA_VIEW->setText(formatNumber(node->videoViewCount));
        }
        ui->INST_MDIA_VIEW->setHidden((node->videoViewCount == 0));
        ui->INST_MDIA_VIEW_ICON->setHidden((node->videoViewCount == 0));

        if (node->commentCount == -1) {
            fitTextToLabel(ui->INST_MDIA_COM, _("NODE_COM"));
            ui->INST_MDIA_COM->setStyleSheet("color: #a8a8a8; font-size: 10pt;");
        } else {
            ui->INST_MDIA_COM->setText(formatNumber(node->commentCount));
            ui->INST_MDIA_COM->setStyleSheet("color: white; font-size: 12pt;");
        }

        if (node->likeCount == -1) {
            fitTextToLabel(ui->INST_MDIA_LIKE, _("NODE_LIKE"));
            ui->INST_MDIA_LIKE->setStyleSheet("color: #a8a8a8; font-size: 10pt;");
            ui->INST_MDIA_VIEW->setVisible(false);
        } else {
            ui->INST_MDIA_LIKE->setText(formatNumber(node->likeCount));
            ui->INST_MDIA_LIKE->setStyleSheet("color: white; font-size: 12pt;");
        }

        ui->INST_WDGT_PRVW_PGS->setCurrentIndex(1);
    }
    ui->LBL_LISA->setVisible((params["user"] == "LISA"));
    if (params["user"] == "LISA") {
        setLabelTextWithEmoji(ui->LBL_LISA, _("LBL_LISA"), "", "lisa-giggling.png");
    }

    params["caption"] = node->caption;
    QString targetTemplate;
    if (node->type == "Story") {
        params["link"] = QString("https://www.instagram.com/stories/%1/").arg(node->foreignOwnerUsername);
        targetTemplate = "instagram_story";
    } else {
        params["caption"] = node->caption;
        params["link"] = QString("https://www.instagram.com/p/%1/").arg(node->shortcode);
        targetTemplate = "instagram_post";
    }
    currentParams = params;
    manager->generateCopyPasteText(targetTemplate, ui->INST_LN_RES, params);
    ui->INST_WDGT_PRVW_PGS->setVisible(true);

}

void MainWindow::updateGeneratedText()
{
    if (!currentSelectedNode) return;

    QString targetTemplate = (currentSelectedNode->type == "Story") ? "instagram_story" : "instagram_post";

    manager->generateCopyPasteText(targetTemplate, ui->INST_LN_RES, currentParams);
}

void MainWindow::resetPreviewWidget()
{
    setLabelTextWithEmoji(ui->INST_LBL_MDST, _("NO_MDIA"), "lisa-think.png");
    currentSelectedNode = nullptr;
    ui->INST_WDGT_PRVW_PGS->setVisible(false);
    ui->INST_LN_RES->clear();
    ui->INST_LV_FEED->clearSelection();
    m_player->stop();
}

void MainWindow::initialLoad()
{

    if (manager->getSettingsStruct().bIsFirstOpen) {

        if (infodialog->isVisible()) {
            infodialog->raise();
            infodialog->activateWindow();
        } else {
            infodialog->show();
        }

        manager->getSettingsStruct().bIsFirstOpen = false;
        manager->saveSettings(manager->getSettingsStruct(), false);
    }
}

void MainWindow::showToast(const QString &message, ToastType type, int durationMs, bool isVideo)
{
    if (!m_toastLabel) {
        m_toastLabel = new QLabel(this);
        m_toastLabel->setAlignment(Qt::AlignCenter);
        m_toastLabel->hide();

        m_toastTimer = new QTimer(this);
        connect(m_toastTimer, &QTimer::timeout, this, &MainWindow::hideToast);
    }

    QFont baseFont = font();
    baseFont.setPointSize(14);
    m_toastLabel->setFont(baseFont);

    QString baseStyle;
    if (type == Warning) {
        baseStyle = "background-color: rgba(40, 35, 20, 220); color: #FFD700; border: 1px solid #5c5029;";
    } else if (type == Error) {
        baseStyle = "background-color: rgba(40, 20, 20, 220); color: #ff6b6b; border: 1px solid #5c2929;";
    } else {
        baseStyle = "background-color: rgba(30, 30, 30, 220); color: white;";
    }

    m_toastLabel->setText(message);

    if (message.trimmed().isEmpty()) {
        m_toastLabel->setStyleSheet("QLabel { " + baseStyle + " padding: 12px 24px; border-radius: 6px; font-size: 14px; min-width: 200px; max-width: 500px; text-align: center; }");
        m_toastLabel->adjustSize();
        m_toastLabel->move((width() - m_toastLabel->width()) / 2, 100);
        m_toastLabel->show();
        m_toastLabel->raise();
        m_toastTimer->stop();
        m_toastTimer->start(durationMs);
        return;
    }

    int maxWidth = qMin(qMax(200, static_cast<int>(width() * 0.9)), 500);

    QFontMetrics fm(baseFont);
    int textWidth = fm.horizontalAdvance(message);

    int finalFontSize = 14;

    if (textWidth > maxWidth) {
        finalFontSize = 14;
        while (finalFontSize > 12) {
            finalFontSize--;
            QFont testFont = baseFont;
            testFont.setPointSize(finalFontSize);
            QFontMetrics testFm(testFont);
            if (testFm.horizontalAdvance(message) <= maxWidth) {
                break;
            }
        }
        finalFontSize = qMax(12, finalFontSize);
    }

    QFont finalFont = baseFont;
    finalFont.setPointSize(finalFontSize);
    m_toastLabel->setFont(finalFont);

    m_toastLabel->setStyleSheet(
        QString("QLabel { "
                "%1"
                "padding: 12px 24px; "
                "border-radius: 6px; "
                "font-size: %2px; "
                "min-width: 200px; "
                "max-width: %3px; "
                "text-align: center; "
                "}")
            .arg(baseStyle)
            .arg(finalFontSize)
            .arg(maxWidth)
        );

    m_toastLabel->adjustSize();
    int offset = 0;
    if (isVideo) offset = 130;
    int x = (width() - m_toastLabel->width()) / 2 - offset;
    qDebug() << "x: " << x << " offset " << offset;
    m_toastLabel->move(x, 100);
    m_toastLabel->show();
    m_toastLabel->raise();

    m_toastTimer->stop();
    m_toastTimer->start(durationMs);
}

void MainWindow::hideToast()
{
    if (m_toastLabel) {
        m_toastLabel->hide();
        m_toastTimer->stop();
    }
}

void MainWindow::on_INST_CMBX_USER_currentIndexChanged(int index)
{
    manager->instagram_setCurrentSelectedUser(index);
    auto user = manager->instagram_getCurrentUserData();
    resetPreviewWidget();
    if (!user->allowGetProfileFeed) {
        ui->INST_WDGT_USER->setCurrentIndex(manager->instagram_getCurrentSelectedUser());
        user->allowUpdateProfileFeedUI = true;
        user->shouldFeedUIRefresh = true;
        updateProfileFeedUI(user);
        return;
    }
    manager->instagram_GET_userInfo(manager->instagram_getCurrentSelectedUser());
    manager->instagram_GET_userFeed(manager->instagram_getCurrentSelectedUser());

}


void MainWindow::on_INST_LV_FEED_clicked(const QModelIndex &index)
{
    if (currentSelectedNode == nullptr || manager->currentUser->feed[index.row()].shortcode != currentSelectedNode->shortcode) {
        ui->INST_WDGT_PRVW_PGS->setHidden(true);
        ui->INST_LBL_MDST->setText(_("LOAD"));
        hideToast();
        QApplication::processEvents(); // Forces UI to be updated since displayNodeContent blocks any queued UI updates until the network requests are done
        displayNodeContent(&manager->currentUser->feed[index.row()]);
    }
}


void MainWindow::on_INST_BTN_NP_clicked()
{
    overlay = new BlockingOverlay(this, "");
    setLabelTextWithEmoji(overlay->m_label, _("INFO_NP"), "star.png");
    overlay->show();
    qApp->processEvents();

    auto user = manager->currentUser;
    user->allowGetProfileFeed = true;
    user->allowUpdateProfileFeedUI = true;
    manager->instagram_GET_userFeed(manager->instagram_getCurrentSelectedUser());
}


void MainWindow::on_INST_BTN_RFSH_clicked()
{
    if ((QDateTime::currentSecsSinceEpoch() - manager->lastApiCall) < 30) {
        manager->warning(_("WARN_RL"), true);
        return;
    }
    manager->m_postCache.clear();
    manager->m_imageCache.clear();
    resetPreviewWidget();
    model->clear();
    manager->currentUser->clear();
    manager->instagram_GET_userInfo(manager->instagram_getCurrentSelectedUser());
    manager->instagram_GET_userFeed(manager->instagram_getCurrentSelectedUser());
    manager->lastApiCall = QDateTime::currentSecsSinceEpoch();
}


void MainWindow::on_INST_LV_POST_clicked(const QModelIndex &index)
{

}


void MainWindow::on_INST_BTN_PLBK_clicked()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
    } else {
        m_player->play();
    }
}


void MainWindow::on_INST_BTN_DOWN_clicked()
{
    if (ui->INST_LN_LINK->text().isEmpty()) return;

    InstagramLinkError error;
    QString shortcode = extractInstagramShortcode(ui->INST_LN_LINK->text(), error);
    switch (error) {
    case InstagramLinkError::Story:
        //qDebug() << "Error: Instagram links are supported, but due to Instagram's limitations, user stories are not.";
        manager->instagram_GET_Story(extractUsernameFromStoriesUrl(ui->INST_LN_LINK->text()), false);
        ui->INST_LBL_MDST->setText(_("LOAD"));
        ui->INST_WDGT_PRVW_PGS->setVisible(false);
        ui->INST_LV_FEED->clearSelection();
        m_player->stop();
        hideToast();
        break;
    case InstagramLinkError::ProfileLink:
        showToast(_("ERR_LINK_PROF"), Error);
        break;
    case InstagramLinkError::InvalidUrl:
    case InstagramLinkError::InvalidPath:
        showToast(_("ERR_LINK_INVL"), Error);
        break;
    case InstagramLinkError::NotInstagram:
        showToast(_("ERR_LINK_INST"), Error);
        break;
    case InstagramLinkError::NoError:
        ui->INST_LBL_MDST->setText(_("LOAD"));
        ui->INST_WDGT_PRVW_PGS->setVisible(false);
        ui->INST_LV_FEED->clearSelection();
        m_player->stop();
        hideToast();
        pendingShortcode = shortcode;
        manager->instagram_GET_PostFromShortcode(shortcode);
        break;
    }
}


void MainWindow::on_INST_LV_POST_doubleClicked(const QModelIndex &index)
{
    if (!currentSelectedNode || index.row() >= currentSelectedNode->children.size()) {
        return;
    }

    MediaViewerDialog *viewer = new MediaViewerDialog(manager, this);

    connect(viewer, &MediaViewerDialog::signal_downloadMedia,
            this, &MainWindow::queueSaveMedia);

    viewer->setAttribute(Qt::WA_DeleteOnClose);
    viewer->displayMediaContent(&currentSelectedNode->children[index.row()]);
    viewer->show();
}


void MainWindow::on_INST_BTN_SOUN_clicked()
{
    if (m_player->audioOutput()->isMuted()) {
        m_player->audioOutput()->setMuted(false);
    } else {
        m_player->audioOutput()->setMuted(true);
    }
}


void MainWindow::on_INST_BTN_SELC_clicked()
{
    ui->INST_LV_POST->selectAll();
}


void MainWindow::on_INST_BTN_DSLC_clicked()
{
    ui->INST_LV_POST->clearSelection();
}

void MainWindow::on_BTN_MGC_clicked()
{

    if (!currentSelectedNode) return;
    if (!currentSelectedNode->children.isEmpty() && currentSelectedNode->children.size() > 10) showToast(_("WARN_SIZE"), Warning, 5000);
    queueSaveMedia();
    if (manager->getSettingsStruct().bAutoCopyText) {
        clipboard->setText(ui->INST_LN_RES->toPlainText());
        showToast(_("INFO_COPY"), Info, 4000, (currentSelectedNode->type == "Video"));
    }
}


void MainWindow::on_BTN_SAVE_clicked()
{

    if (!currentSelectedNode || currentSelectedNode->type != "MediaDict") return;

    QMap<int, Instagram::contentChild> map;

    QItemSelectionModel *selectionModel = ui->INST_LV_POST->selectionModel();
    QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

    qDebug() << selectedIndexes.size();

    if (selectedIndexes.size() == 0) return;

    if (selectedIndexes.size() > 10) {
        showToast(_("WARN_SIZE"), Warning, 5000);
    }

    std::sort(selectedIndexes.begin(), selectedIndexes.end());
    int counter = 0;

    for (const QModelIndex &index : selectedIndexes) {
        int nodeChildIndex = index.row();

        if (nodeChildIndex < 0 || nodeChildIndex >= currentSelectedNode->children.size()) {
            continue;
        }
        const Instagram::contentChild &child = currentSelectedNode->children.values().at(nodeChildIndex);

        map[counter] = child;
        counter++;

    }

    queueSaveMedia(map);
}


void MainWindow::on_MENU_OPEN_SETT_triggered()
{
    if (settingswindow->isVisible()) {
        settingswindow->raise();
        settingswindow->activateWindow();
    } else {
        settingswindow->sw_show();
    }
}


void MainWindow::on_INST_LISA_STRY_clicked()
{
    ui->INST_LBL_MDST->setText(_("LOAD"));
    ui->INST_WDGT_PRVW_PGS->setVisible(false);
    ui->INST_LV_FEED->clearSelection();
    m_player->stop();
    hideToast();
    displayNodeContent(&manager->m_storyCache["lalalalisa_m"]);
}


void MainWindow::on_INST_LOUD_STRY_clicked()
{
    ui->INST_LBL_MDST->setText(_("LOAD"));
    ui->INST_WDGT_PRVW_PGS->setVisible(false);
    ui->INST_LV_FEED->clearSelection();
    m_player->stop();
    hideToast();
    displayNodeContent(&manager->m_storyCache["wearelloud"]);
}


void MainWindow::on_INST_LFAM_STRY_clicked()
{
    ui->INST_LBL_MDST->setText(_("LOAD"));
    ui->INST_WDGT_PRVW_PGS->setVisible(false);
    ui->INST_LV_FEED->clearSelection();
    m_player->stop();
    hideToast();
    displayNodeContent(&manager->m_storyCache["lalala_lfamily"]);
}


void MainWindow::on_MENU_OPEN_INFO_triggered()
{
    if (infodialog->isVisible()) {
        infodialog->raise();
        infodialog->activateWindow();
    } else {
        infodialog->show();
    }
}

