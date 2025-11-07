#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QObject>
#include <QObject>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QAbstractItemModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QPainterPath>
#include <QIcon>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QLayout>
#include <QDir>
#include <QSaveFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QPushButton>
#include <QtNetwork>
#include <QtMath>
#include <QLabel>
#include <QTextEdit>
#include <QDesktopServices>

#include "lang.h"

enum class InstagramLinkError {
    NoError,
    InvalidUrl,
    NotInstagram,
    Story,
    ProfileLink,
    InvalidPath
};

enum Position { Left, Right };

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Critical
};

QString getDateFormat(int format, bool backupFormat = false);
QString translate(const QString &key, int lang);
QString formatNumber(int number);
QString formatTimestampWithOrdinal(qint64 timestamp);
void fitTextToLabel(QLabel *target, const QString &text);
QString extractInstagramShortcode(const QString &url, InstagramLinkError &error);
QString extractUsernameFromStoriesUrl(const QString &url);
QString extractFullnameFromACPT(const QString &acpt);
void setPixmapToText(QLabel *textLabel, QLabel *pixmapLabel, Position position, int offset = 20, bool adjustY = false);
void fitTextToButton(QPushButton *button, const QString &text);
QString reflowTextWithPrefix(const QString &text, const QString &prefix, const QFontMetrics &fm, int maxWidth);
void setLabelTextWithEmoji(QLabel *label, const QString &text,
                           const QString &emojiBefore = QString(),
                           const QString &emojiAfter = QString());

// utils.h

class Logger : public QObject
{
    Q_OBJECT
public:
    static Logger* instance();

    void setParentWidget(QWidget *parent, int dateFormat);

    void log(const QString &message, bool MB = false, QWidget *parent = nullptr);           // Info (Standard)
    void warning(const QString &message, bool MB = true, QWidget *parent = nullptr); // Warning
    void critical(const QString &message, bool MB = true, QWidget *parent = nullptr); // Cricital
    void debug(const QString &message);         // Debug
private:
    QFile logFile;
    QTextStream logStream;
    QWidget *messageParent = nullptr;
    Logger();

    void logMessage(LogLevel level, const QString &message, bool MessageBox, QWidget *parent = nullptr);
    QString levelToString(LogLevel level);

    int intDateFormat = 0;
};


#endif // UTILS_H
