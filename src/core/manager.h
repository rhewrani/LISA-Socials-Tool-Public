#ifndef MANAGER_H
#define MANAGER_H

#define _(key) (manager->t(key))

#include "fileagent.h"
#include "instagram.h"

#include <QObject>
#include <QApplication>
#include <QProcess>

#include <QtGui>


class MainWindow;

class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(MainWindow* mainWindowRef, QObject *parent = nullptr);
    ~Manager();

    void debug(const QString &message);
    void log(const QString &message, bool MessageBox = false);
    void warning(const QString &message, bool MessageBox = true);
    void critical(const QString &message, bool MessageBox = true);

    QString t(const QString &key);

    QNetworkAccessManager networkManager;

    struct appSettings {
        bool bEnableLogging = false;
        bool bOpenFileExplorerOnSave = true;
        bool bEnableDiscordQuoting = true;
        bool bAutoCopyText = true;
        bool bIsFirstOpen = true;
        int intLanguage = 0;
        QString strDownloadDir;
        QString strSessionid;
        QString strVersion;
        QMap<QString, QString> presets;

        appSettings() = default;

        appSettings(const appSettings &other) = default;
    };

    void GetSettings();
    appSettings& getSettingsStruct() { return settings; }
    bool saveSettings(appSettings settings, bool restart);

    bool saveMedia(const QPixmap &pixmap, const QString &path);
    bool saveMediaVideo(const QString &videoUrl, const QString &path);

    void loadPixmap(const QString &url, const QString &id, int w, int h,
                    std::function<void(const QPixmap&)> callback);

    void generateCopyPasteText(const QString &presetKey, QTextEdit *target, const QMap<QString, QString> &params);
    void generateCopyPasteTextString(const QString &templateText, QTextEdit *target, const QMap<QString, QString> &params, bool enableQuoting = true);

    /* INSTAGRAM LOGIC */
    void instagram_setCurrentSelectedUser(int user) { instagram_currentSelectedInstagramUser = user; currentUser = instagram->getUserPtr(instagram_currentSelectedInstagramUser);}
    int instagram_getCurrentSelectedUser() { return instagram_currentSelectedInstagramUser; }
    Instagram::userData* instagram_getCurrentUserData() { return instagram->getUserPtr(instagram_currentSelectedInstagramUser); }

    void instagram_GET_userInfo(int user);
    void instagram_GET_userFeed(int user);
    void instagram_GET_PostFromShortcode(QString &shortcode);
    void instagram_GET_Story(const QString &username, bool isAutoFetch = true); // Autofetch is true at program init and when switching + refreshing the user info. This basically tells the program to not instantly display the story when fetched

    Instagram::userData *currentUser = nullptr;

    QHash<QString, QPixmap> m_imageCache;
    QHash<QString, Instagram::contentNode> m_postCache;
    QHash<QString, Instagram::contentNode> m_storyCache;

    int lastApiCall = 0; // Only for those that need to be rate limited

signals:

private:
    MainWindow* mainWindow;
    FileAgent *fileAgent;
    Instagram *instagram;

    void Init();
    void InitInstagram();

    const QPixmap* getCachedPixmap(const QString &id) const;
    void cachePixmap(const QString &id, const QPixmap &pixmap);
    bool isPixmapCached(const QString &id) const;


    appSettings settings;
    Instagram::userData *lisaStruct = nullptr;
    Instagram::userData *lloudStruct = nullptr;
    Instagram::userData *lfamilyStruct = nullptr;

    int instagram_currentSelectedInstagramUser = 0; // 0 = LISA, 1 = LLOUD, 2 = LFAMILY // ALSO CALLED userIndex
};

#endif // MANAGER_H
