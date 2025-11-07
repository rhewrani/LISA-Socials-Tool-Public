#include "utils.h"
#include <QStringList>
#include <QDebug>
#include <QString>
#include <QChar>

#include <QMovie>



QString translate(const QString &key, int lang)
{
    switch (lang) {
    case 0:  // English
        return texts_en.value(key, key);
    case 1:  // German
        return texts_de.value(key, key);
    case 2:  // Danish
        return texts_da.value(key, key);
    case 3: // Dutch
        return texts_nl.value(key, key);
    case 4:  // French
        return texts_fr.value(key, key);
    case 5:  // Spanish
        return texts_es.value(key, key);
    case 6: // Portuguese
        return texts_pt.value(key, key);
    case 7:  // Italian
        return texts_it.value(key, key);
    case 8:  // Chinese
        return texts_zh.value(key, key);
    case 9:  // Japanese
        return texts_jp.value(key, key);
    case 10:  // Korean
        return texts_kr.value(key, key);
    case 11: // Thai
        return texts_th.value(key, key);
    default:
        return key;  // Fallback: Key itself
    }
}

QString formatNumber(int number) {

    if (number < 1000) {
        return QString::number(number);
    }

    if (number < 10000) {
        QString str = QString::number(number);
        return str.insert(str.length() - 3, QChar(','));
    }

    if (number < 1000000) {
        int thousands = number / 1000;
        int remainder = number % 1000;
        int decimalDigit = remainder / 100;

        if (decimalDigit == 0) {
            return QString::number(thousands) + QStringLiteral("K");
        } else {
            return QString::number(thousands) + QStringLiteral(",") +
                   QChar('0' + decimalDigit) + QStringLiteral("K");
        }
    } else {
        int millions = number / 1000000;
        int remainder = number % 1000000;
        int decimalDigit = remainder / 100000;

        if (decimalDigit == 0) {
            return QString::number(millions) + QStringLiteral("M");
        } else {
            return QString::number(millions) + QStringLiteral(",") +
                   QChar('0' + decimalDigit) + QStringLiteral("M");
        }
    }
}

QString formatTimestampWithOrdinal(qint64 timestamp)
{
    QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp, Qt::UTC);
    int day = dt.date().day();
    QString ordinal;
    if (day >= 11 && day <= 13) {
        ordinal = "th";
    } else {
        switch (day % 10) {
        case 1: ordinal = "st"; break;
        case 2: ordinal = "nd"; break;
        case 3: ordinal = "rd"; break;
        default: ordinal = "th"; break;
        }
    }

    QLocale locale(QLocale::English);
    QString month = locale.toString(dt.date(), "MMMM");
    QString time = dt.time().toString("hh:mm");
    return QString("%1 %2%3, %4, %5 UTC")
        .arg(month)
        .arg(day)
        .arg(ordinal)
        .arg(dt.date().year())
        .arg(time);
}

QString getDateFormat(int format, bool backupFormat)
{
    if (backupFormat) {
        switch (format) {
        case 0: return "dd-MM-yyyy_hh-mm-ss";
        case 1: return "MM-dd-yyyy_hh-mm-ss";
        case 2: return "yyyy-MM-dd_hh-mm-ss";
        default: return "dd-MM-yyyy_hh-mm-ss";
        }
    } else {
        switch (format) {
        case 0: return "dd-MM-yyyy hh:mm:ss";
        case 1: return "MM-dd-yyyy hh:mm:ss";
        case 2: return "yyyy-MM-dd hh:mm:ss";
        default: return "dd-MM-yyyy hh:mm:ss";
        }
    }
}


void fitTextToLabel(QLabel *target, const QString &text)
{
    if (!target) return;

    target->setText(text);
    QFont font = target->font();
    const int originalFontSize = font.pointSize();
    const int minFontSize = 5;


    font.setPointSize(originalFontSize);
    target->setFont(font);


    QFontMetrics fm(font);
    QRect boundingRect = fm.boundingRect(target->rect(), target->wordWrap() ? Qt::TextWordWrap : 0, text);


    if (boundingRect.width() <= target->width() &&
        boundingRect.height() <= target->height()) {
        return;
    }


    for (int size = originalFontSize - 1; size >= minFontSize; --size) {
        font.setPointSize(size);
        target->setFont(font);
        fm = QFontMetrics(font);
        boundingRect = fm.boundingRect(target->rect(), target->wordWrap() ? Qt::TextWordWrap : 0, text);

        if (boundingRect.width() <= target->width() &&
            boundingRect.height() <= target->height()) {
            return;
        }
    }


    font.setPointSize(minFontSize);
    target->setFont(font);
}

void fitTextToButton(QPushButton *button, const QString &text)
{
    if (!button || text.isEmpty()) return;

    QFont baseFont = button->font();
    int originalSize = baseFont.pointSize();
    const int minFontSize = 8;

    QStyleOptionButton opt;
    opt.initFrom(button);
    opt.text = text;
    opt.icon = button->icon();
    QRect contentsRect = button->style()->subElementRect(QStyle::SE_PushButtonContents, &opt, button);

    if (!button->icon().isNull()) {
        contentsRect.setLeft(contentsRect.left() + 24);
    }

    int availableWidth = contentsRect.width();
    int availableHeight = contentsRect.height();

    if (availableWidth <= 0 || availableHeight <= 0) return;

    QFont font = baseFont;
    font.setPointSize(originalSize);
    button->setFont(font);
    button->setText(text);

    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(text);

    if (textWidth <= availableWidth) {
        return;
    }

    for (int size = originalSize - 1; size >= minFontSize; --size) {
        font.setPointSize(size);
        fm = QFontMetrics(font);
        if (fm.horizontalAdvance(text) <= availableWidth) {
            button->setFont(font);
            return;
        }
    }

    font.setPointSize(minFontSize);
    button->setFont(font);
}

void setLabelTextWithEmoji(QLabel *label, const QString &text,
                           const QString &emojiBefore,
                           const QString &emojiAfter)
{
    if (!label) return;

    QString htmlText = text;
    htmlText.replace("\n", "<br>");

    auto embedImage = [](const QString &path) -> QString {
        if (path.isEmpty()) return QString();
        if (QPixmap(path).isNull()) return QString();
        return QString("<img src=\"%1\" width=\"%2\" height=\"%2\" style=\"vertical-align: middle;\"/>")
            .arg(path).arg(32);
    };

    QString before = embedImage(":/emojis/" + emojiBefore);
    QString after = embedImage(":/emojis/" + emojiAfter);

    if (!before.isEmpty()) before += "&nbsp;&nbsp;";
    if (!after.isEmpty()) htmlText += "&nbsp;&nbsp;";

    QString richText = before + htmlText + after;

    label->setTextFormat(Qt::RichText);
    label->setText(richText);
}

QString extractUsernameFromStoriesUrl(const QString &url)
{
    QUrl qurl(url.trimmed());
    if (!qurl.isValid()) return {};

    if (qurl.host() != "www.instagram.com" && qurl.host() != "instagram.com")
        return {};

    QString path = qurl.path();
    if (!path.startsWith("/stories/"))
        return {};

    QString remainder = path.mid(9);
    if (remainder.endsWith('/'))
        remainder.chop(1);

    int slashIndex = remainder.indexOf('/');
    if (slashIndex != -1) {
        remainder = remainder.left(slashIndex);
    }

    if (remainder.isEmpty())
        return {};

    QRegularExpression usernameRegex(R"(^[a-zA-Z0-9_.]{1,30}$)");
    if (!usernameRegex.match(remainder).hasMatch())
        return {};

    return remainder;
}

QString extractFullnameFromACPT(const QString &acpt)
{
    QRegularExpression regex("^Photo by ([^\\s]+)");
    QRegularExpressionMatch match = regex.match(acpt);
    if (match.hasMatch()) {
        return match.captured(1);
    }
    return QString();
}

QString extractInstagramShortcode(const QString &url, InstagramLinkError &error)
{
    error = InstagramLinkError::NoError;
    QUrl qurl(url.trimmed());

    if (!qurl.isValid() || qurl.scheme() != "https") {
        error = InstagramLinkError::InvalidUrl;
        return {};
    }

    if (qurl.host() != "www.instagram.com" && qurl.host() != "instagram.com") {
        error = InstagramLinkError::InvalidUrl;
        return {};
    }

    QString path = qurl.path();
    if (path.isEmpty()) {
        error = InstagramLinkError::InvalidUrl;
        return {};
    }

    if (path.endsWith('/'))
        path.chop(1);


    if (path.startsWith("/stories/")) {
        error = InstagramLinkError::Story;
        return {};
    }


    QStringList parts = path.split('/', Qt::SkipEmptyParts);

    if (parts.isEmpty()) {
        error = InstagramLinkError::InvalidPath;
        return {};
    }

    QString shortcode;


    if (parts[0] == "p" || parts[0] == "reel") {
        if (parts.size() < 2) {
            error = InstagramLinkError::InvalidPath;
            return {};
        }
        shortcode = parts[1];
    }

    else if (parts.size() >= 3 && (parts[1] == "p" || parts[1] == "reel")) {
        QRegularExpression usernameRegex(R"(^[a-zA-Z0-9_\.]{1,30}$)");
        if (!usernameRegex.match(parts[0]).hasMatch()) {
            error = InstagramLinkError::InvalidPath;
            return {};
        }
        shortcode = parts[2];
    }

    else if (parts.size() == 1) {
        QRegularExpression usernameRegex(R"(^[a-zA-Z0-9_\.]{1,30}$)");
        if (usernameRegex.match(parts[0]).hasMatch()) {
            error = InstagramLinkError::ProfileLink;
            return {};
        }
        error = InstagramLinkError::InvalidPath;
        return {};
    }
    else {
        error = InstagramLinkError::InvalidPath;
        return {};
    }


    if (shortcode.contains('?')) {
        shortcode = shortcode.left(shortcode.indexOf('?'));
    }
    if (shortcode.contains('#')) {
        shortcode = shortcode.left(shortcode.indexOf('#'));
    }


    QRegularExpression shortcodeRegex(R"(^[a-zA-Z0-9_-]{1,20}$)");
    if (shortcode.isEmpty() || !shortcodeRegex.match(shortcode).hasMatch()) {
        error = InstagramLinkError::InvalidPath;
        return {};
    }

    return shortcode;
}

void setPixmapToText(QLabel *textLabel, QLabel *pixmapLabel, Position position, int offset, bool adjustY)
{
    if (!textLabel || !pixmapLabel) return;

    if (!textLabel->parent() || !pixmapLabel->parent()) {
        qWarning() << "Both labels must have the same parent widget.";
        return;
    }

    textLabel->show();
    pixmapLabel->show();

    QFontMetrics fm(textLabel->font());
    int textWidth = fm.horizontalAdvance(textLabel->text());

    QPoint textPos = textLabel->pos();
    int textY = textPos.y();
    int pixmapHeight = pixmapLabel->sizeHint().height();
    int pixmapY = adjustY ? textY + (textLabel->height() - pixmapHeight) / 2 : pixmapLabel->pos().y();

    if (position == Right) {

        int pixmapX = textPos.x() + textWidth + offset;
        pixmapLabel->move(pixmapX, pixmapY);
    } else {

        int pixmapX = textPos.x() - pixmapLabel->width() - offset;
        pixmapLabel->move(pixmapX, pixmapY);
    }
}

QString reflowTextWithPrefix(const QString &text, const QString &prefix, const QFontMetrics &fm, int maxWidth)
{
    if (text.isEmpty() || maxWidth <= 0) {
        return prefix + text;
    }

    QStringList lines;
    QStringList paragraphs = text.split('\n');

    for (const QString &paragraph : paragraphs) {
        if (paragraph.isEmpty()) {
            lines << prefix;
            continue;
        }

        QString currentLine;
        QStringList words = paragraph.split(' ');

        for (const QString &word : words) {
            QString testLine = currentLine.isEmpty() ? word : currentLine + " " + word;
            int width = fm.horizontalAdvance(testLine);

            if (width > maxWidth && !currentLine.isEmpty()) {
                lines << prefix + currentLine;
                currentLine = word;
            } else {
                currentLine = testLine;
            }
        }

        if (!currentLine.isEmpty()) {
            lines << prefix + currentLine;
        }
    }

    return lines.join('\n');
}


Logger* Logger::instance()
{
    static Logger instance;
    return &instance;
}

Logger::Logger()
{
    logFile.setFileName("log.txt");
    if (!logFile.open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Can't open log file!";
        return;
    }
    logStream.setDevice(&logFile);
    logStream << "\n=== New Logging-session started: "
              << QDateTime::currentDateTime().toString(getDateFormat(intDateFormat))
              << " ===\n";
    logStream.flush();
}

void Logger::setParentWidget(QWidget *parent, int dateFormat)
{
    messageParent = parent;
    intDateFormat = dateFormat;
}

void Logger::log(const QString &message, bool MB, QWidget *parent)
{
    logMessage(LogLevel::Info, message, MB);
}

void Logger::warning(const QString &message, bool MB, QWidget *parent)
{
    logMessage(LogLevel::Warning, message, MB, parent);
}

void Logger::critical(const QString &message, bool MB, QWidget *parent)
{
    logMessage(LogLevel::Critical, message, MB, parent);
}

void Logger::debug(const QString &message)
{
    logMessage(LogLevel::Debug, message, false);
}

void Logger::logMessage(LogLevel level, const QString &message, bool MessageBox, QWidget *parent)
{
    QString timestamp = QDateTime::currentDateTime().toString(getDateFormat(intDateFormat));
    QString levelStr = levelToString(level);

    logStream << "[" << timestamp << "][" << levelStr << "] " << message << "\n";
    logStream.flush();

    if (level == LogLevel::Debug) {
        qDebug() << message;
    }

    if (MessageBox == true && (level != LogLevel::Debug)) {
        QWidget *useParent = parent ? parent : messageParent;

        if (useParent) {
            if (level == LogLevel::Warning) {
                QMessageBox::warning(useParent, "Warning", message);
            } else if (level == LogLevel::Critical) {
                QMessageBox::critical(useParent, "Critical", message);
            } else {
                QMessageBox::information(useParent, "Information", message);
            }
        }
    }
}

QString Logger::levelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:    return "DEBUG";
    case LogLevel::Info:     return "INFO";
    case LogLevel::Warning:  return "WARN";
    case LogLevel::Critical: return "CRIT";
    default:                 return "INFO";
    }
}

