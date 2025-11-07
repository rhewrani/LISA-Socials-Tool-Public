#ifndef INSTAGRAM_H
#define INSTAGRAM_H

#include <QObject>
#include "fileagent.h"

class Instagram : public QObject
{
    Q_OBJECT
public:
    explicit Instagram(FileAgent *fileAgentRef, QNetworkAccessManager &networkManagerRef, int lang, QString sessionid, QObject *parent = nullptr);

    struct contentChild {
        QString type = "Image"; // Default for child
        QString mediaUrl;
        QString videoUrl;
        QString id;
        QString accessabilityCaption;
        QString story_timestamp;
        QString story_expires;
        int dimensionHeight;
        int dimensionWidth;
        int childIndex;
    };

    struct contentNode {
        QString shortcode;
        QString type = "Image"; // Default for feed node. If it's a carousel it's set to "MediaDict" and with videos/reel to "Video"
        QString imageUrl; // Display and download url for single-image posts and MediaDicts (with MediaDicts it's just the first image in the carousel which is used as a thumbnail)
        QString videoUrl; // Display and download url for reels (also for reels in feed)
        QString location;
        QString caption;
        QString accessabilityCaption;
        QString timestamp;
        QString id;
        QString foreignOwnerUsername; // foreign is used if post is fetched using link
        QString foreignOwnerFullname;
        QString foreignOwnerPfpUrl;
        QString foreignOwnerId;
        bool foreignOwnerIsVerified = false;
        bool isNew = false;
        int originalDimensionHeight = 0; // Check whether this is needed
        int originalDimensionWidth = 0; // Check whether this is needed
        int videoUrlHeight = 0; // Might be able to delete Height/Width for video since it's unused
        int videoUrlWidth = 0;
        int videoViewCount = 0;
        int likeCount = 0;
        int commentCount = 0;
        QMap<int, contentChild> children; // Post children, only if type is sidecar

    };

    struct userData {
        QString username;
        QString fullname;
        QString biography;
        QString profilePicUrl;
        QString endCursor; // Cursor for pagination
        QString id;
        int followersCount = 0;
        int postsCount = 0;
        int currentFeedIndex = 0;
        bool allowGetProfileInfo = true;
        bool allowGetProfileFeed = true;
        bool allowUpdateProfileInfoUI = true;
        bool allowUpdateProfileFeedUI = true;
        bool shouldFeedUIRefresh = true;
        bool hasNextPage = false;
        QMap<int, contentNode> feed;
        QList<contentNode> appendFeed;

        void clear();
        void dump();
    };

    userData* getUserPtr(int userIndex);

    void GET_userInfo(userData *user);
    void GET_userFeed(userData *user);
    void GET_post(const QString &shortcode, QHash<QString, contentNode> &hash);
    void GET_story(const QString &username, QHash<QString, contentNode> &hash, bool isAutoFetch);

    QString t(const QString &key);

signals:

    void signal_updateMainPageProfileInfo(Instagram::userData *user, bool loadStory);
    void signal_updateMainPageProfileFeed(Instagram::userData *user);
    void signal_postFetched(const QString &shortcode);
    void signal_storyFetched(const QString &username, bool isAutoFetch);
    void signal_fetchFailed();

private:
    FileAgent *fileAgent;
    QNetworkAccessManager &networkManager; // Main networkManager used for the program
    // temporary networkManagers created used for feed and post fetch on each call
    // the reason for this is some kind of weird behavior with QNetworkAccessManager. It's only able to make one authenticated api call to instagrams api before failing
    // this is currently fixed by just creating a temporary QNetworkAccessManager for each call (which is automatically deleted afterwards) and works just as good
    // the issue is probably related with the connection protocol, both http1 and http2 being used, etc.

    void Init();
    QJsonObject getObjectFromEntries(const QString& name, const QString& data);
    QJsonObject findReelsInObject(const QJsonObject &obj);
    QJsonObject findReelsInArray(const QJsonArray &arr);
    void generateSessionData(int isInit = false);
    void setupHeaders(QNetworkRequest &request, int headerSet);
    void extractFeedData(QJsonArray &arr, userData* user);
    contentNode extractPostData(QJsonObject &obj);
    contentNode extractStoryData(const QJsonObject &obj);
    QJsonObject extractReelsMedia(const QString &htmlContent);
    bool checkResponse(QNetworkReply *reply, const QString &origin, int currentAttempt = 1);


    QString anonCookie;
    QString currentLsdToken;
    QString currentCsrfToken;
    QString appId = "936619743392459";
    // For each of these there is a max attempts cap at 1. They are placed where the respective function usually fails
    int postFetchAttempts = 0;
    int feedFetchAttempts = 0;
    int storyFetchAttempts = 0;
    int settingsLanguage = 0;
    QString settingsSessionid;

    userData LISA = { .username = "lalalalisa_m", .id = "8012033210" };
    userData LLOUD = { .username = "wearelloud", .id = "64466054435" };
    userData LFAMILY = { .username = "lalala_lfamily", .id = "49500655974" };

};

class FeedListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IsVideoRole = Qt::UserRole + 1,
        IsNewRole
    };

    explicit FeedListModel(QObject *parent = nullptr);
    ~FeedListModel() override = default;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void clear();
    void setFeed(const QMap<int, Instagram::contentNode> &feed);
    void appendPosts(const QList<Instagram::contentNode> &newPosts);
    void setPixmapForRow(int row, const QPixmap &pixmap);
    bool hasPixmapForRow(int row) const;

private:
    QList<Instagram::contentNode> m_feed;
    QVector<QPixmap> m_pixmaps;
};


class ChildMediaModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IsVideoRole = Qt::UserRole + 1
    };

    explicit ChildMediaModel(QObject *parent = nullptr);
    ~ChildMediaModel() override = default;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setChildren(const QMap<int, Instagram::contentChild> &children);
    void setPixmapForRow(int row, const QPixmap &pixmap);
    bool hasPixmapForRow(int row) const;

private:
    QList<Instagram::contentChild> m_children;
    QVector<QPixmap> m_pixmaps;
};


class FeedItemDelegate : public QStyledItemDelegate {
public:


    explicit FeedItemDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class VideoOverlayDelegate : public QStyledItemDelegate
{
public:
    explicit VideoOverlayDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // INSTAGRAM_H
