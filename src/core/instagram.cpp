#include "instagram.h"
#include <QPixmapCache>


Instagram::Instagram(FileAgent *fileAgentRef, QNetworkAccessManager &networkManagerRef, int lang, QString sessionid, QObject *parent)
    : QObject{parent}
    , fileAgent{fileAgentRef}
    , networkManager{networkManagerRef}
    , settingsLanguage(lang)
    , settingsSessionid(sessionid)
{
    Init();
}


void Instagram::GET_userInfo(userData *user)
{

    if (!user->allowGetProfileInfo) return;

    QString urlString = "https://www.instagram.com/api/v1/users/web_profile_info/?username=%1";
    QUrl url(urlString.arg(user->username));
    QNetworkRequest request(url);

    setupHeaders(request, -1);
    QNetworkReply *reply = networkManager.get(request);

    connect(reply, &QNetworkReply::finished, [this, reply, user] {

        user->allowGetProfileInfo = false;

        if(!checkResponse(reply, "[USER_INFO]")) return;

        QByteArray responseData = reply->readAll();

        if (responseData.isEmpty()) {
            Logger::instance()->critical("Response is empty!");
            reply->deleteLater();
            return;
        }

        QJsonParseError jsonError;
        QJsonDocument doc = QJsonDocument::fromJson(responseData, &jsonError);

        if (jsonError.error != QJsonParseError::NoError) {
            Logger::instance()->critical("JSON Parse Error: " + jsonError.errorString());
            reply->deleteLater();
            return;
        }
        QJsonObject res = doc.object();
        QJsonObject data = res.value("data").toObject();
        if (data.isEmpty()) {
            Logger::instance()->critical("No 'data' object found in GraphQL response.");
            reply->deleteLater();
            return;
        }
        QJsonObject userObj = data.value("user").toObject();
        user->fullname = userObj.value("full_name").toString();
        user->biography = userObj.value("biography").toString();
        user->profilePicUrl = userObj.value("profile_pic_url").toString();
        user->followersCount = userObj.value("edge_followed_by").toObject().value("count").toInt();
        user->postsCount = userObj.value("edge_owner_to_timeline_media").toObject().value("count").toInt();

        reply->deleteLater();

        emit signal_updateMainPageProfileInfo(user, true);

    });
}

void Instagram::GET_userFeed(userData *user)
{
    if (!user->allowGetProfileFeed) return;

    QString username = user->username;

    QString urlString = QString("https://www.instagram.com/%1/").arg(username);

    QUrl url(urlString);

    QString afterValue = user->hasNextPage ? QString("\"%1\"").arg(user->endCursor) : "null";

    QString variables = QString(

                            "{\"after\":%1,"

                            "\"before\":null,"

                            "\"data\":{\"count\":12,\"include_reel_media_seen_timestamp\":true,\"include_relationship_info\":true,\"latest_besties_reel_media\":true,\"latest_reel_media\":true},"

                            "\"username\":\"%2\","

                            "\"__relay_internal__pv__PolarisIsLoggedInrelayprovider\":true}"

                            ).arg(afterValue, user->username);
    // count: 12 is the amount of posts to fetch with one request


    QString encodedVars = QUrl::toPercentEncoding(variables);

    QString body = QString("variables=%1&doc_id=31850238601258116&fb_api_caller_class=RelayModern&fb_api_req_friendly_name=PolarisProfilePostsQuery&server_timestamps=true").arg(encodedVars);

    QUrl graphqlUrl("https://www.instagram.com/graphql/query/");

    QNetworkRequest graphqlRequest(graphqlUrl);

    setupHeaders(graphqlRequest, 2);

    QNetworkAccessManager *tempManager = new QNetworkAccessManager(this);
    QNetworkReply *graphqlReply = tempManager->post(graphqlRequest, body.toUtf8());
    connect(graphqlReply, &QNetworkReply::finished, tempManager, &QObject::deleteLater);

    connect(graphqlReply, &QNetworkReply::finished, [this, graphqlReply, user] {

    user->allowGetProfileFeed = false;

    if(!checkResponse(graphqlReply, "[FEED]", feedFetchAttempts)) {
        if (feedFetchAttempts < 1) {
            feedFetchAttempts++;
            Logger::instance()->critical("Failed to get feed data, attempting again...", false);
            GET_userFeed(user);
        }
        return;
    }
    QByteArray responseData = graphqlReply->readAll();

    if (responseData.isEmpty()) {
        Logger::instance()->critical("Error extracting feed data. Please see log.txt for more information. [FEED-ER]");
        Logger::instance()->critical("Response is empty!", false);
        graphqlReply->deleteLater();
        return;
    }

    QJsonParseError jsonError;
    QJsonDocument jsonResponseDoc = QJsonDocument::fromJson(responseData, &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        Logger::instance()->critical("Error extracting feed data. Please see log.txt for more information. [FEED-GRQL]");
        Logger::instance()->critical("Error parsing GraphQL JSON:" + jsonError.errorString(), false);
        graphqlReply->deleteLater();
        return;
    }

    if (!jsonResponseDoc.isObject()) {
        Logger::instance()->critical("Error extracting feed data. Please see log.txt for more information. [FEED-GRQL_NOTJSON]");
        Logger::instance()->critical("GraphQL response is not a JSON object.", false);
        graphqlReply->deleteLater();
        return;
    }

    QJsonObject jsonResponse = jsonResponseDoc.object();
    if (jsonResponse.contains("errors") && jsonResponse["errors"].isArray()) {
        QJsonArray errors = jsonResponse["errors"].toArray();
        if (!errors.isEmpty()) {
            QJsonDocument errorDoc(errors);
            QString errorString = errorDoc.toJson(QJsonDocument::Indented);
            Logger::instance()->critical("Error extracting feed data. Please see log.txt for more information. [FEED-ER_JSON]");
            Logger::instance()->critical("GraphQL API Errors:\n" + errorString, false);
            graphqlReply->deleteLater();
            return;
        }
    }

    QJsonObject dataObj = jsonResponse.value("data").toObject();
    if (dataObj.isEmpty()) {
        Logger::instance()->critical("Error extracting feed data. Please see log.txt for more information. [FEED-NO_DATA]");
        Logger::instance()->critical("No 'data' object found in GraphQL response.", false);
        graphqlReply->deleteLater();
        return;
     }

    QJsonObject feedConnectionObj = dataObj.value("xdt_api__v1__feed__user_timeline_graphql_connection").toObject();
    if (feedConnectionObj.isEmpty()) {
        Logger::instance()->critical("Error extracting feed data. Please see log.txt for more information. [FEED-NO_XDT]");
        Logger::instance()->critical("No 'xdt_api__v1__feed__user_timeline_graphql_connection' object found in GraphQL response data.", false);
        graphqlReply->deleteLater();
        return;
    }

    QJsonArray feedEdgesArray = feedConnectionObj.value("edges").toArray();
    if (feedEdgesArray.isEmpty()) {
        Logger::instance()->critical("Error extracting feed data. Please see log.txt for more information. [FEED-NO_EDGES]");
        Logger::instance()->critical("No 'edges' array found in 'xdt_api__v1__feed__user_timeline_graphql_connection'.", false);
        graphqlReply->deleteLater();
        return;
    }


    QJsonObject pageInfoObj = feedConnectionObj.value("page_info").toObject();
    if (!pageInfoObj.isEmpty()) {
        user->hasNextPage = pageInfoObj.value("has_next_page").toBool();
        user->endCursor = pageInfoObj.value("end_cursor").toString();
    }

    feedFetchAttempts = 0;
    extractFeedData(feedEdgesArray, user);

    });
}

void Instagram::GET_post(const QString &shortcode, QHash<QString, contentNode> &hash)
{

    QString variables = QString("{\"shortcode\":\"%1\",\"fetch_tagged_user_count\":null,\"hoisted_comment_id\":null,\"hoisted_reply_id\":null}")
                            .arg(shortcode);

    QString encodedVars = QUrl::toPercentEncoding(variables);

    QString body = QString("variables=%1&doc_id=8845758582119845&fb_api_caller_class=RelayModern&fb_api_req_friendly_name=PolarisPostActionLoadPostQueryQuery&server_timestamps=true")
                       .arg(encodedVars);

    QUrl graphqlUrl("https://www.instagram.com/graphql/query/");
    QNetworkRequest graphqlRequest(graphqlUrl);

    setupHeaders(graphqlRequest, 2);

    QNetworkAccessManager *tempManager = new QNetworkAccessManager(this);
    QNetworkReply *graphqlReply = tempManager->post(graphqlRequest, body.toUtf8());
    connect(graphqlReply, &QNetworkReply::finished, tempManager, &QObject::deleteLater);

    connect(graphqlReply, &QNetworkReply::finished, [this, graphqlReply, shortcode, &hash] {

        if(!checkResponse(graphqlReply, "[POST]")) return;

        QByteArray responseData = graphqlReply->readAll();

        // Instead of throwing critical error, show error messages within the mainwindow instead

        if (responseData.isEmpty()) {
            Logger::instance()->critical("Error extracting post data. Please see log.txt for more information. [POST-ER]");
            Logger::instance()->critical("Response is empty!", false);
            emit signal_fetchFailed();
            graphqlReply->deleteLater();
            return;
        }

        QJsonParseError jsonError;
        QJsonDocument jsonResponseDoc = QJsonDocument::fromJson(responseData, &jsonError);

        if (jsonError.error != QJsonParseError::NoError) {
            Logger::instance()->critical("Error extracting post data. Please see log.txt for more information. [POST-GRQL]");
            Logger::instance()->critical("Error parsing GraphQL JSON:" + jsonError.errorString(), false);
            emit signal_fetchFailed();
            graphqlReply->deleteLater();
            return;
        }

        if (!jsonResponseDoc.isObject()) {
            Logger::instance()->critical("Error extracting post data. Please see log.txt for more information. [POST-GRQL_NOTJSON]");
            Logger::instance()->critical("GraphQL response is not a JSON object.", false);
            emit signal_fetchFailed();
            graphqlReply->deleteLater();
            return;
        }

        QJsonObject jsonResponse = jsonResponseDoc.object();
        if (jsonResponse.contains("errors") && jsonResponse["errors"].isArray()) {
            QJsonArray errors = jsonResponse["errors"].toArray();
            if (!errors.isEmpty()) {
                QJsonDocument errorDoc(errors);
                QString errorString = errorDoc.toJson(QJsonDocument::Indented);
                Logger::instance()->critical("Error extracting post data. Please see log.txt for more information. [POST-ER_JSON]");
                Logger::instance()->critical("GraphQL API Errors:\n" + errorString, false);
                emit signal_fetchFailed();
                graphqlReply->deleteLater();
                return;
            }
        }

        QJsonObject dataObj = jsonResponse.value("data").toObject();
        if (dataObj.isEmpty()) {
            Logger::instance()->critical("Error extracting post data. Please see log.txt for more information. [POST-NODATA]");
            Logger::instance()->critical("No 'data' object found in GraphQL response.", false);
            emit signal_fetchFailed();
            graphqlReply->deleteLater();
            return;
        }

        QJsonObject postObj = dataObj.value("xdt_shortcode_media").toObject();
        if (postObj.isEmpty()) {
            if (postFetchAttempts < 1) { // max 1 more attempt before ultimately failing
                postFetchAttempts++;
                Logger::instance()->critical("Failed to get post data, attempting again...", false);
                GET_post(shortcode, hash);
            } else {
                Logger::instance()->critical(t("ERR_FETCH_FAIL_POST") + "[POST-NO_XDT]");
                Logger::instance()->critical("No 'xdt_shortcode_media' object found in GraphQL response data.", false);
                emit signal_fetchFailed();
            }
            graphqlReply->deleteLater();
            return;
        }

        contentNode post = extractPostData(postObj);
        hash[shortcode] = post;
        postFetchAttempts = 0;
        emit signal_postFetched(shortcode);

    });
}

void Instagram::GET_story(const QString &username, QHash<QString, contentNode> &hash, bool isAutoFetch)
{

    QUrl url(QString("https://www.instagram.com/stories/%1/?r=1").arg(username));


    QNetworkRequest request(url);

    request.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7");
    request.setRawHeader("Cookie", ("sessionid=" + settingsSessionid).toUtf8());
    // Due to instagrams API limitation, fetching stories is limited to being logged in which can be simulated using a sessionid

    QNetworkReply *reply = networkManager.get(request);

    connect(reply, &QNetworkReply::finished, [this, reply, username, &hash, isAutoFetch] {

        if (!checkResponse(reply, "[STORY]")) {
            emit signal_fetchFailed();
            return;
        }

        QByteArray response = reply->readAll();
        if (response.isEmpty()) {
            Logger::instance()->critical("Error extracting story data. Please see log.txt for more information. [STORY-ER]");
            Logger::instance()->critical("Response is empty!", false);
            emit signal_fetchFailed();
            reply->deleteLater();
            return;
        }

        QJsonObject reelsMedia = extractReelsMedia(QString::fromUtf8(response));

        if (reelsMedia.isEmpty()) {
            // to-do: add indicator which notifies the user to make sure the profile isn't private and has a story
            if (!isAutoFetch) {
                if (storyFetchAttempts < 1) {
                    storyFetchAttempts++;
                    Logger::instance()->critical("Failed to get story data, attempting again...", false);
                    GET_story(username, hash, false);
                } else {
                    Logger::instance()->critical(t("ERR_FETCH_FAIL_STRY") + "[STORY-NO_REELS]");
                    Logger::instance()->critical("Could not find 'xdt_api__v1__feed__reels_media' in HTML response. This also occurs if the story is private.", false);
                    emit signal_fetchFailed();
                }
            }
            reply->deleteLater();
            return;
        }

        if (!reelsMedia.contains("reels_media") || !reelsMedia["reels_media"].isArray() || reelsMedia["reels_media"].toArray().isEmpty()) {
            if (!isAutoFetch) {
                Logger::instance()->critical("Error extracting story data. Please see log.txt for more information. [STORY-NO_REELS_ARRAY]");
                Logger::instance()->critical("'reels_media' field missing or not an array.", false);
                emit signal_fetchFailed();
            }
            reply->deleteLater();
            return;
        }

        QJsonArray reelsArray = reelsMedia["reels_media"].toArray();
        contentNode story = extractStoryData(reelsArray.first().toObject());
        hash[username] = story;
        storyFetchAttempts = 0;
        emit signal_storyFetched(username, isAutoFetch);

    });
}

QString Instagram::t(const QString &key)
{
    return translate(key, settingsLanguage);
}


void Instagram::Init()
{
    generateSessionData(true);

}

QJsonObject Instagram::getObjectFromEntries(const QString &name, const QString &data)
{
    QRegularExpression regex(R"(\[")" + QRegularExpression::escape(name) + R"(",.*?,(\{.*?\}),\d+\])");
    QRegularExpressionMatch match = regex.match(data);

    if (match.hasMatch()) {
        QString objStr = match.captured(1);
        QJsonDocument doc = QJsonDocument::fromJson(objStr.toUtf8());
        if (doc.isObject()) {
            return doc.object();
        }
    }
    return QJsonObject(); // Return empty object if nothing is found
}

QJsonObject Instagram::findReelsInArray(const QJsonArray &arr)
{
    for (const QJsonValue &elem : arr) {
        if (elem.isObject()) {
            QJsonObject nested = findReelsInObject(elem.toObject());
            if (!nested.isEmpty())
                return nested;
        } else if (elem.isArray()) {
            QJsonObject nested = findReelsInArray(elem.toArray());
            if (!nested.isEmpty())
                return nested;
        }
    }
    return QJsonObject();
}

QJsonObject Instagram::findReelsInObject(const QJsonObject &obj)
{
    if (obj.contains("xdt_api__v1__feed__reels_media")) {
        return obj.value("xdt_api__v1__feed__reels_media").toObject();
    }

    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        const QJsonValue &val = it.value();
        if (val.isObject()) {
            QJsonObject nested = findReelsInObject(val.toObject());
            if (!nested.isEmpty())
                return nested;
        } else if (val.isArray()) {
            QJsonObject found = findReelsInArray(val.toArray());
            if (!found.isEmpty())
                return found;
        }
    }
    return QJsonObject();
}

QJsonObject Instagram::extractReelsMedia(const QString &htmlContent)
{
    QRegularExpression scriptRegex(
        R"(<script\s+type\s*=\s*["']application/json["'][^>]*>(.*?)<\/script>)",
        QRegularExpression::DotMatchesEverythingOption
        );

    QRegularExpressionMatchIterator it = scriptRegex.globalMatch(htmlContent);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString jsonStr = match.captured(1).trimmed();

        if (jsonStr.isEmpty())
            continue;

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);

        if (parseError.error != QJsonParseError::NoError)
            continue;

        if (!doc.isObject())
            continue;

        QJsonObject obj = doc.object();

        QJsonObject found = findReelsInObject(obj);
        if (!found.isEmpty())
            return found;
    }

    return QJsonObject(); // Not found
}


void Instagram::generateSessionData(int isInit)
{
    QUrl url("https://www.instagram.com/lalalalisa_m/");
    QNetworkRequest request(url);
    setupHeaders(request, 0);

    QNetworkReply *reply = networkManager.get(request);

    connect(reply, &QNetworkReply::finished, [this, reply, isInit] {

        if (!checkResponse(reply, "[SESSION]")) return;

        QString responseString(reply->readAll());

        QJsonObject polarisSiteData = getObjectFromEntries("PolarisSiteData", responseString);
        QJsonObject webConfig = getObjectFromEntries("DGWWebConfig", responseString);
        QJsonObject lsdObj = getObjectFromEntries("LSD", responseString);
        QJsonObject securityConfig = getObjectFromEntries("InstagramSecurityConfig", responseString);

        currentLsdToken = lsdObj.value("token").toString();
        if (currentLsdToken.isEmpty()) {
            currentLsdToken = QUuid::createUuid().toRfc4122().toBase64(QByteArray::Base64UrlEncoding).left(11);
        }
        currentCsrfToken = securityConfig.value("csrf_token").toString();
        if (!webConfig.value("appId").toString().isEmpty()) {
            appId = webConfig.value("appId").toString().toUtf8().constData();
        }

        // Create anonCookie
        QStringList anonCookieParts;
        if (!currentCsrfToken.isEmpty()) {
            anonCookieParts.append("csrftoken=" + currentCsrfToken);
        }
        if (!polarisSiteData.value("device_id").toString().isEmpty()) {
            anonCookieParts.append("ig_did=" + polarisSiteData.value("device_id").toString());
        }
        anonCookieParts.append("wd=1280x720");
        anonCookieParts.append("dpr=2");
        if (!polarisSiteData.value("machine_id").toString().isEmpty()) {
            anonCookieParts.append("mid=" + polarisSiteData.value("machine_id").toString());
        }
        anonCookieParts.append("ig_nrcb=1");

        anonCookie = anonCookieParts.join("; ");

        if (isInit) {
            GET_userInfo(&LISA);
            GET_userFeed(&LISA);
        }

        reply->deleteLater();

    });

}

void Instagram::setupHeaders(QNetworkRequest &request, int headerSet)
{
    if (headerSet == 0) { // Common headers
        request.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7");
        request.setRawHeader("Accept-Language", "en-GB,en;q=0.9");
        request.setRawHeader("Cache-Control", "max-age=0");
        request.setRawHeader("Dnt", "1");
        request.setRawHeader("Priority", "u=0, i");
        request.setRawHeader("Sec-Ch-Ua", "\"Chromium\";v=\"124\", \"Google Chrome\";v=\"124\", \"Not-A.Brand\";v=\"99\"");
        request.setRawHeader("Sec-Ch-Ua-Mobile", "?0");
        request.setRawHeader("Sec-Ch-Ua-Platform", "Windows");
        request.setRawHeader("Sec-Fetch-Dest", "document");
        request.setRawHeader("Sec-Fetch-Mode", "navigate");
        request.setRawHeader("Sec-Fetch-Site", "none");
        request.setRawHeader("Sec-Fetch-User", "?1");
        request.setRawHeader("Upgrade-Insecure-Requests", "1");
        request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36");
        return;
    } else if (headerSet == 1) { // Session headers with anonCookie, lsd, etc.
        request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
        request.setRawHeader("x-ig-app-id", appId.toUtf8().constData());
        if (appId.isEmpty()) {
            appId = "936619743392459";
            request.setRawHeader("x-ig-app-id", appId.toUtf8().constData());
        }
        request.setRawHeader("X-FB-LSD", currentLsdToken.toUtf8().constData());
        if (!currentCsrfToken.isEmpty()) {
            request.setRawHeader("X-CSRFToken", currentCsrfToken.toUtf8().constData());
        }
        request.setRawHeader("cookie", anonCookie.toUtf8().constData());
        return;
    } else if (headerSet == 2) { // headers used for graphql which include session data
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        request.setRawHeader("X-IG-App-ID", "936619743392459");
        request.setRawHeader("X-CSRFToken", currentCsrfToken.toUtf8());
        request.setRawHeader("Cookie", ("sessionid=" + settingsSessionid).toUtf8());
    }
    // Basic headers
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Accept-Language", "en-US,en;q=0.9");
    request.setRawHeader("x-ig-app-id", appId.toUtf8().constData());
    return;
}

void Instagram::extractFeedData(QJsonArray &arr, userData *user)
{
    for (const auto &feedEdgeValue : arr) {

        if (!feedEdgeValue.isObject()) {
            Logger::instance()->critical(QString("Feed node at index %1 is not an object.").arg(user->currentFeedIndex), false);
            continue;
        }

        QJsonObject feedEdgeObj = feedEdgeValue.toObject();
        QJsonObject nodeObj = feedEdgeObj.value("node").toObject();
        if (nodeObj.isEmpty()) {
            Logger::instance()->critical(QString("Feed node at index %1 is empty.").arg(user->currentFeedIndex));
            continue;
        }

        contentNode feedNode;

        feedNode.shortcode = nodeObj.value("code").toString();

        QJsonObject userObj = nodeObj.value("user").toObject();
        feedNode.foreignOwnerUsername = userObj.value("username").toString();
        feedNode.foreignOwnerFullname = userObj.value("full_name").toString();
        feedNode.foreignOwnerPfpUrl = userObj.value("profile_pic_url").toString();
        feedNode.foreignOwnerId =  userObj.value("id").toString();
        feedNode.foreignOwnerIsVerified = userObj.value("is_verified").toBool();

        if (nodeObj.value("commenting_disabled_for_viewer").toBool() == true) {
            feedNode.commentCount = -1; // -1 means disabled
        } else {
            if (!nodeObj.value("comment_count").isNull()) {
                feedNode.commentCount = nodeObj.value("comment_count").toInt();
            }
        }

        if (nodeObj.value("like_and_view_counts_disabled").toBool() == true) {
                feedNode.likeCount = -1;
        } else {
            if (!nodeObj.value("like_count").isNull()) {
                 feedNode.likeCount = nodeObj.value("like_count").toInt();
            }
        }

        feedNode.id = nodeObj.value("id").toString();

        feedNode.originalDimensionHeight = nodeObj.value("original_height").toInt();
        feedNode.originalDimensionWidth = nodeObj.value("original_width").toInt();

        if (!nodeObj.value("taken_at").isNull()) {
            feedNode.timestamp = formatTimestampWithOrdinal(nodeObj.value("taken_at").toInt());
            feedNode.isNew = ((QDateTime::currentSecsSinceEpoch() - nodeObj.value("taken_at").toInt()) < 86400);
        }

        if (!nodeObj.value("location").isNull()) {
            feedNode.location = nodeObj.value("location").toString();
        }

        if (!nodeObj.value("caption").isNull()) {
            QJsonObject captionObj = nodeObj.value("caption").toObject();
            feedNode.caption = captionObj.value("text").toString();
        }

        if (!nodeObj.value("accessibility_caption").isNull()) {
            feedNode.accessabilityCaption = nodeObj.value("accessibility_caption").toString();
        }

        if(!nodeObj.value("image_versions2").isNull()) {
            QJsonArray candidates = nodeObj.value("image_versions2").toObject().value("candidates").toArray(); // Pretty dangerous since no safety checks but whatever (praying meta doesn't change their response structure)
            if (!candidates.isEmpty()) {
                // access first element since first since it has the hightest resolution
                QJsonObject imgUrl = candidates.first().toObject();
                feedNode.imageUrl = imgUrl["url"].toString().trimmed();
            }

        }

        if (!nodeObj.value("video_versions").isNull()) {
            // Entire Post is just a video/reel
            feedNode.type = "Video";
            QJsonObject videoObj = nodeObj["video_versions"].toArray().first().toObject();
            feedNode.videoUrlWidth =videoObj.value("width").toInt();
            feedNode.videoUrlHeight =videoObj.value("height").toInt();
            feedNode.videoUrl = videoObj.value("url").toString().trimmed();


        } else if (!nodeObj.value("carousel_media").isNull()) {
            feedNode.type = "MediaDict";
                QJsonArray carousel = nodeObj.value("carousel_media").toArray();
                int counter = 0;
                for (const auto &carouselNode : carousel) {
                    QJsonObject carouselObj = carouselNode.toObject();
                    contentChild child;

                    child.childIndex = counter;
                    child.id = carouselObj.value("id").toString();
                    child.dimensionHeight = carouselObj.value("original_height").toInt();
                    child.dimensionWidth = carouselObj.value("original_width").toInt();

                    if (!carouselObj.value("accessibility_caption").isNull()) {
                        child.accessabilityCaption = carouselObj.value("accessibility_caption").toString();
                    }

                    if (!carouselObj.value("video_versions").isNull()) {
                        child.type = "Video";
                        QJsonObject videoObj = carouselObj["video_versions"].toArray().first().toObject();
                        child.videoUrl = videoObj.value("url").toString().trimmed();
                    }

                    QJsonArray candidates = carouselObj.value("image_versions2").toObject().value("candidates").toArray();
                    if (!candidates.isEmpty()) {
                        QJsonObject imgUrl = candidates.first().toObject();
                        child.mediaUrl = imgUrl["url"].toString().trimmed();
                    }


                    feedNode.children[counter] = child;
                    counter++;
                }
        }

        user->feed[user->currentFeedIndex] = feedNode;
        user->appendFeed.append(feedNode);
        user->currentFeedIndex++;
    }

    emit signal_updateMainPageProfileFeed(user);
}

Instagram::contentNode Instagram::extractPostData(QJsonObject &postObj)
{
    contentNode post;

    post.shortcode = postObj.value("shortcode").toString();
    if (postObj.value("comments_disabled").toBool() == true) {
        post.commentCount = -1;
    } else {
        if (!postObj.value("edge_media_preview_comment").isNull()) {
            post.commentCount = postObj.value("edge_media_preview_comment").toObject().value("count").toInt();
        }
    }
    if (postObj.value("like_and_view_counts_disabled").toBool() == true) {
        post.likeCount = -1;
    } else {
        if (!postObj.value("edge_media_preview_like").isNull()) {
            post.likeCount = postObj.value("edge_media_preview_like").toObject().value("count").toInt();
        }
    }

    post.id = postObj.value("id").toString();

    QJsonObject ownerObj = postObj.value("owner").toObject();
    post.foreignOwnerUsername = ownerObj.value("username").toString();
    post.foreignOwnerFullname = ownerObj.value("full_name").toString();
    post.foreignOwnerPfpUrl = ownerObj.value("profile_pic_url").toString();
    post.foreignOwnerId =  ownerObj.value("id").toString();
    post.foreignOwnerIsVerified = ownerObj.value("is_verified").toBool();


    QJsonObject dimensionObj = postObj.value("dimensions").toObject();
    post.originalDimensionHeight = dimensionObj.value("height").toInt();
    post.originalDimensionWidth = dimensionObj.value("width").toInt();

    if (!postObj.value("taken_at_timestamp").isNull()) {
        post.timestamp = formatTimestampWithOrdinal(postObj.value("taken_at_timestamp").toInt());
    }

    if (!postObj.value("location").isNull()) {
        post.location = postObj.value("location").toString();
    }

    QJsonArray captionParentEdges = postObj.value("edge_media_to_caption").toObject().value("edges").toArray();
    if (!captionParentEdges.isEmpty()) {
        QJsonObject captionNode = captionParentEdges.first().toObject().value("node").toObject();
        post.caption = captionNode.value("text").toString();
    }

    if (!postObj.value("accessibility_caption").isNull()) {
        post.accessabilityCaption = postObj.value("accessibility_caption").toString();
    }

    post.imageUrl = postObj.value("display_url").toString();

    if (postObj.value("__typename").toString() == "XDTGraphVideo") {
        post.type = "Video";
        post.videoUrl = postObj.value("video_url").toString();
        post.videoViewCount = postObj.value("video_play_count").toInt();

    } else if (postObj.value("__typename").toString() == "XDTGraphSidecar") {
        post.type = "MediaDict";
        QJsonArray postChildrenEdges = postObj.value("edge_sidecar_to_children").toObject().value("edges").toArray();
        int counter = 0;
        for (const auto &childNode : postChildrenEdges) {
            QJsonObject childObj = childNode.toObject().value("node").toObject();
            contentChild child;

            child.childIndex = counter;
            child.id = QString("%1_%2").arg(childObj.value("id").toString()).arg(post.foreignOwnerId);

            QJsonObject dimensionObj = childObj.value("dimensions").toObject();
            child.dimensionHeight = dimensionObj.value("height").toInt();
            child.dimensionWidth = dimensionObj.value("width").toInt();

            if (!childObj.value("accessibility_caption").isNull()) {
                child.accessabilityCaption = childObj.value("accessibility_caption").toString();
            }

            child.mediaUrl = childObj.value("display_url").toString();
            if (childObj.value("is_video").toBool() == true) {
                child.type = "Video";
                child.videoUrl = childObj.value("video_url").toString();
            }

            post.children[counter] = child;
            counter++;
        }
    }
    return post;


}

Instagram::contentNode Instagram::extractStoryData(const QJsonObject &storyObj)
{
    contentNode story;

    QJsonObject userObj = storyObj.value("user").toObject();
    story.foreignOwnerUsername = userObj.value("username").toString();
    story.foreignOwnerFullname = userObj.value("full_name").toString();
    story.foreignOwnerPfpUrl = userObj.value("profile_pic_url").toString();
    story.foreignOwnerId =  userObj.value("id").toString();
    story.foreignOwnerIsVerified = userObj.value("is_verified").toBool();

    story.type = "Story";

    QJsonArray storyItems = storyObj.value("items").toArray();
    int counter = 0;
    for (const auto &childNode : storyItems) {
        QJsonObject childObj = childNode.toObject();
        contentChild child;

        child.childIndex = counter;
        child.id = childObj.value("id").toString();

        if (!childObj.value("accessibility_caption").isNull() && story.foreignOwnerFullname.isEmpty()) {
            story.foreignOwnerFullname = extractFullnameFromACPT(childObj.value("accessibility_caption").toString());
        }

        if (!childObj.value("taken_at").isNull()) {
            child.story_timestamp = formatTimestampWithOrdinal(childObj.value("taken_at").toInt());
        }

        if (!childObj.value("expiring_at").isNull()) {
            child.story_expires = formatTimestampWithOrdinal(childObj.value("expiring_at").toInt());
        }

        child.dimensionHeight = childObj.value("original_height").toInt();
        child.dimensionWidth = childObj.value("original_width").toInt();

        if (!childObj.value("accessibility_caption").isNull()) {
            child.accessabilityCaption = childObj.value("accessibility_caption").toString();
        }

        child.mediaUrl = childObj.value("image_versions2").toObject().value("candidates").toArray().first().toObject().value("url").toString();
        if (!childObj.value("video_versions").isNull()) {
            child.type = "Video";
            child.videoUrl = childObj.value("video_versions").toArray().first().toObject().value("url").toString();
        }

        story.children[counter] = child;
        counter++;
    }

    return story;
}

bool Instagram::checkResponse(QNetworkReply *reply, const QString &origin, int currentAttempt)
{
    if (reply->error()) {
        if (currentAttempt > 0) {
            Logger::instance()->critical(t("ERR_FTCH_FAIL") + " " + origin);
            Logger::instance()->critical("Response: " + reply->errorString(), false);
            Logger::instance()->critical("If response is 'Host requires authentication', wait a few minutes before trying again", false);
        }
        reply->deleteLater();
        return false;
    }

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (statusCode != 200) {
        QByteArray responseData = reply->readAll();
        Logger::instance()->critical("Couldn't fetch data from Instagram API: Request failed with status code:" + QString::number(statusCode) + "\nPlease see log.txt for more information." + origin);
        Logger::instance()->critical("Response: " + QString(responseData), false);
        reply->deleteLater();
        return false;
    }

    return true;
}

Instagram::userData *Instagram::getUserPtr(int userIndex)
{
    if (userIndex == 0) {
        return &LISA;
    } else if (userIndex == 1) {
        return &LLOUD;
    } else if (userIndex == 2) {
        return &LFAMILY;
    }

    return &LISA;
}

FeedListModel::FeedListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int FeedListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_feed.size();
}

QVariant FeedListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_feed.size())
        return {};

    const auto &node = m_feed[index.row()];

    switch (role) {
    case IsVideoRole:
        return !node.videoUrl.isEmpty();
    case IsNewRole:
        return node.isNew;
    case Qt::DecorationRole:
        if (index.row() < m_pixmaps.size() && !m_pixmaps[index.row()].isNull())
            return m_pixmaps[index.row()];
        return {};
    default:
        return {};
    }
}

QHash<int, QByteArray> FeedListModel::roleNames() const
{
    return {
        {IsVideoRole, "isVideo"},
        {IsNewRole, "isNew"}
    };
}

void FeedListModel::clear()
{
    if (m_feed.isEmpty()) return;
    beginResetModel();
    m_feed.clear();
    m_pixmaps.clear();
    endResetModel();
}

void FeedListModel::setFeed(const QMap<int, Instagram::contentNode> &feed)
{
    beginResetModel();
    m_feed = feed.values();
    m_pixmaps.clear();
    endResetModel();
}

void FeedListModel::appendPosts(const QList<Instagram::contentNode> &newPosts)
{
    if (newPosts.isEmpty()) return;
    int first = m_feed.size();
    int last = first + newPosts.size() - 1;
    beginInsertRows({}, first, last);
    m_feed << newPosts;
    m_pixmaps.resize(m_pixmaps.size() + newPosts.size());
    endInsertRows();
}

void FeedListModel::setPixmapForRow(int row, const QPixmap &pixmap)
{
    if (row < 0 || row >= m_feed.size())
        return;

    if (m_pixmaps.size() <= row)
        m_pixmaps.resize(row + 1);

    m_pixmaps[row] = pixmap;

    QModelIndex idx = index(row);
    emit dataChanged(idx, idx, {Qt::DecorationRole});
}

bool FeedListModel::hasPixmapForRow(int row) const
{
    return row >= 0 && row < m_pixmaps.size() && !m_pixmaps[row].isNull();
}

ChildMediaModel::ChildMediaModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ChildMediaModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_children.size();
}

QVariant ChildMediaModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_children.size())
        return {};

    const auto &child = m_children[index.row()];
    switch (role) {
    case IsVideoRole:
        return !child.videoUrl.isEmpty();
    case Qt::DecorationRole:
        if (index.row() < m_pixmaps.size() && !m_pixmaps[index.row()].isNull())
            return m_pixmaps[index.row()];
        return {};

    default:
        return {};
    }
}

QHash<int, QByteArray> ChildMediaModel::roleNames() const
{
    return {
        {IsVideoRole, "isVideo"}
    };
}

void ChildMediaModel::setChildren(const QMap<int, Instagram::contentChild> &children)
{
    beginResetModel();
    m_children = children.values();
    m_pixmaps.clear();
    endResetModel();
}

void ChildMediaModel::setPixmapForRow(int row, const QPixmap &pixmap)
{
    if (row < 0 || row >= m_children.size()) return;
    if (m_pixmaps.size() <= row)
        m_pixmaps.resize(row + 1);
    m_pixmaps[row] = pixmap;
    emit dataChanged(index(row), index(row), {Qt::DecorationRole});
}

bool ChildMediaModel::hasPixmapForRow(int row) const
{
    return row >= 0 && row < m_pixmaps.size() && !m_pixmaps[row].isNull();
}

QSize FeedItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(170, 230);
}

void FeedItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    painter->save();

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, QColor(60, 140, 220, 180));
    }

    QPixmap pixmap = index.data(Qt::DecorationRole).value<QPixmap>();
    if (!pixmap.isNull()) {
        QRect drawRect = option.rect.adjusted(2, 2, -2, -2);
        painter->drawPixmap(drawRect, pixmap.scaled(
                                          drawRect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation
                                          ));
    }

    if (index.data(FeedListModel::IsVideoRole).toBool()) {
        QPixmap playIcon(":/images/play.png");
        if (!playIcon.isNull()) {
            int size = qMin(option.rect.width(), option.rect.height()) / 5;
            QRect iconRect(
                option.rect.right() - size - 6,
                option.rect.bottom() - size - 6,
                size, size
                );
            painter->drawPixmap(iconRect, playIcon.scaled(
                                              iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation
                                              ));
        }
    }

    if (index.data(FeedListModel::IsNewRole).toBool()) {
        painter->save();
        painter->setPen(QColor(33, 196, 44));
        painter->setFont(QFont("Arial", 13, QFont::Bold));
        painter->drawText(option.rect.adjusted(6, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, "NEW");
        painter->restore();
    }

    painter->restore();
}

void VideoOverlayDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    painter->save();

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, QColor(60, 140, 220, 180));
    }

    QPixmap pixmap = index.data(Qt::DecorationRole).value<QPixmap>();
    if (!pixmap.isNull()) {
        QRect drawRect = option.rect.adjusted(2, 2, -2, -2);
        painter->drawPixmap(drawRect, pixmap.scaled(
                                          drawRect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation
                                          ));
    }

    if (index.data(ChildMediaModel::IsVideoRole).toBool()) {
        QPixmap playIcon(":/images/play.png");
        if (!playIcon.isNull()) {
            int size = qMin(option.rect.width(), option.rect.height()) / 5;
            QRect iconRect(
                option.rect.right() - size - 6,
                option.rect.bottom() - size - 6,
                size, size
                );
            painter->drawPixmap(iconRect, playIcon.scaled(
                                              iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation
                                              ));
        }
    }

    painter->restore();
}

QSize VideoOverlayDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(170, 230);
}

void Instagram::userData::clear()
{

    feed.clear();
    appendFeed.clear();
    allowGetProfileInfo = true;
    allowGetProfileFeed = true;
    allowUpdateProfileInfoUI = true;
    allowUpdateProfileFeedUI = true;
    shouldFeedUIRefresh = true;
    hasNextPage = false;
    endCursor.clear();
    currentFeedIndex = 0;
    postsCount = 0;
    followersCount = 0;
}

void Instagram::userData::dump()
{
    qDebug() << "USER DATA DUMP FOR " << username;
    qDebug() << "FULLNAME: " << fullname;
    qDebug() << "BIOGRAPHY: " << biography;
    qDebug() << "PROFILE PIC URL: " << profilePicUrl;
    qDebug() << "ENDCURSOR: " << endCursor;
    qDebug() << "FOLLOWERS COUNT: " << followersCount;
    qDebug() << "POSTS COUNT: " << postsCount;
    qDebug() << "CURRNT FEED INDEX: " << currentFeedIndex;
    qDebug() << "ALLOW GET PROFILE INFO: " << allowGetProfileInfo;
    qDebug() << "ALLOW GET PROFILE FEED: " << allowGetProfileFeed;;
    qDebug() << "ALLOW UPDATE PROFILE INFO UI: " << allowUpdateProfileInfoUI;
    qDebug() << "ALLOW UPDATE PROFILE FEED UI: " << allowUpdateProfileFeedUI;
    qDebug() << "SHOULD FEED UI REFRESH: " << shouldFeedUIRefresh;
    qDebug() << "HAS NEXT PAGE " << hasNextPage;
    qDebug() << "FEED SIZE: " << feed.size();
    qDebug() << "APPEND FEED SIZE:  " << appendFeed.size();

}
