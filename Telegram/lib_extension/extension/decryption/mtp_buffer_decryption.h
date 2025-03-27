#include "message_text_decryption.h"
#include "base/basic_types.h"
#include "styles/style_info.h"
#include "mtproto/core_types.h"
#include "scheme.h"

#pragma once

// using mtpBuffer = QVector<mtpPrime>;
void decrypt_the_buffer(mtpBuffer& buffer);