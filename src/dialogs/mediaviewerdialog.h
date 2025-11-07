#ifndef MEDIAVIEWERDIALOG_H
#define MEDIAVIEWERDIALOG_H

#include <QDialog>
#include "../core/manager.h"

class Manager;

namespace Ui {
class MediaViewerDialog;
}

class MediaViewerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MediaViewerDialog(Manager* managerRef, QWidget *parent = nullptr);
    ~MediaViewerDialog();

    void displayMediaContent(Instagram::contentChild *node);
    void displayPfp(Instagram::contentChild child);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

signals:
    void signal_downloadMedia(const QMap<int, Instagram::contentChild> map);

private slots:
    void on_BTN_PLBK_clicked();

    void on_BTN_SOUN_clicked();

    void on_BTN_DWLD_clicked();

private:
    Ui::MediaViewerDialog *ui;
    Manager* manager;
    QMediaPlayer *m_player;
    QVideoWidget *m_videoWidget;
    QAudioOutput *m_audioOutput;

    Instagram::contentChild displayChild;

    QPoint m_dragStartPosition;
    bool m_isDragging = false;
    QWidget *titleBar = nullptr;

    void Init();
    void InitTitleBar();
    void InitLang();
};

#endif // MEDIAVIEWERDIALOG_H
