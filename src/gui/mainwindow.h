#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "../core/manager.h"
#include "../dialogs/mediaviewerdialog.h"
#include "../dialogs/infodialog.h"
#include "../dialogs/blockingoverlay.h"
#include "settingswindow.h"
#include "clickablelabel.h"

enum ToastType { Info, Warning, Error };

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void updateProfileInfoUI(Instagram::userData *user, bool loadStory = false);
    void updateProfileFeedUI(Instagram::userData *user);
    void displayNodeContent(Instagram::contentNode *node);
    void toggleStoryButton(const QString &username);
    void updateGeneratedText();
    void resetPreviewWidget();
    void initialLoad();

    void showToast(const QString &message, ToastType type = Info, int durationMs = 3000, bool isVideo = false);

    BlockingOverlay *overlay;

    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:


    void on_INST_CMBX_USER_currentIndexChanged(int index);

    void on_INST_LV_FEED_clicked(const QModelIndex &index);

    void on_INST_BTN_NP_clicked();

    void on_INST_BTN_RFSH_clicked();

    void on_INST_LV_POST_clicked(const QModelIndex &index);

    void on_INST_BTN_PLBK_clicked();

    void on_INST_BTN_DOWN_clicked();

    void on_INST_LV_POST_doubleClicked(const QModelIndex &index);

    void on_INST_BTN_SOUN_clicked();

    void on_INST_BTN_SELC_clicked();

    void on_INST_BTN_DSLC_clicked();

    void on_BTN_MGC_clicked();

    void on_BTN_SAVE_clicked();

    void on_MENU_OPEN_SETT_triggered();

    void on_INST_LISA_STRY_clicked();

    void on_INST_LOUD_STRY_clicked();

    void on_INST_LFAM_STRY_clicked();

    void on_MENU_OPEN_INFO_triggered();

    void hideToast();

private:
    Ui::MainWindow *ui;
    Manager *manager;
    Settingswindow *settingswindow;
    MediaViewerDialog* mediaViewer;
    Infodialog *infodialog;
    FeedListModel* model;
    ChildMediaModel* cModel;
    QMediaPlayer *m_player;
    QVideoWidget *m_videoWidget;
    QAudioOutput *m_audioOutput;

    QClipboard *clipboard;

    ClickableLabel *INST_MDCT_PFP;
    ClickableLabel *INST_MDIA_PFP;

    Instagram::contentNode *currentSelectedNode = nullptr;
    QMap<QString, QString> currentParams;

    QString pendingShortcode;
    bool preventPlaybackLoop = false;

    QPoint m_dragStartPosition;
    bool m_isDragging = false;
    QWidget *titleBar = nullptr;

    QLabel *m_toastLabel = nullptr;
    QTimer *m_toastTimer = nullptr;

    void Init();
    void InitTitleBar();
    void InitUI();
    void InitLang();

    void setPfpImageFromURL(QLabel *target, QString &url, QString &id, int dimX, int dimY);
    void downloadImagesForFeed(const QList<Instagram::contentNode> &posts, int startRow);
    void downloadChildMediaImages(const QMap<int, Instagram::contentChild> &children);
    void generateCopyPasteText(const QString &presetKey, QTextEdit *target, const QMap<QString, QString> &params);
    void queueSaveMedia(const QMap<int, Instagram::contentChild> &map = {});
    void savePfp(const Instagram::contentChild *child);
    void openPfpViewer();
};

#endif // MAINWINDOW_H
