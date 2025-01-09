#pragma once
#include <iostream>
#include <QtCore/QString>
#include <QtCore/QVector>

// QString make_encrypted_message(const TextWithTags& text_with_tags, const PeerData* peer);
// QString make_encrypted_message(const QString& text_with_tags, const quint64 peer);

QString encrypt_the_message(const QString& text, const quint64 peer_id);
