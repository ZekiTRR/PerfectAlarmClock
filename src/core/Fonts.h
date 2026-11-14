#pragma once

#include <QFont>
#include <QString>

namespace pac::fonts {

[[nodiscard]] QString uiFamily();
[[nodiscard]] QString digitFamily();
[[nodiscard]] QFont uiFont(qreal pointSize, QFont::Weight weight = QFont::Normal);
[[nodiscard]] QFont digitFont(qreal pointSize, QFont::Weight weight = QFont::Normal);

[[nodiscard]] QString loadFontFile(const QString& path);
void initialize();
void apply();

} // namespace pac::fonts
