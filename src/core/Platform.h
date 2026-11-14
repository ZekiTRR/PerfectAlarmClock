#pragma once

#include <QString>

namespace pac::platform {

void setAutostart(bool enabled);
[[nodiscard]] bool autostartEnabled();
[[nodiscard]] QString dataDirectory();
void revealInExplorer(const QString& path);
void flashTaskbar();

} // namespace pac::platform
