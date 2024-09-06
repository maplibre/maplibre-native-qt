// Copyright (C) 2023 MapLibre contributors

// SPDX-License-Identifier: BSD-2-Clause

#include "image_parameter.hpp"

#include "style_parameter.hpp"

namespace QMapLibre {

/*!
    \class ImageParameter
    \brief A helper utility to manage image sources.
    \ingroup QMapLibre

    \headerfile image_parameter.hpp <QMapLibre/ImageParameter>
*/

/*!
    \brief Default constructor
*/
ImageParameter::ImageParameter(QObject *parent)
    : StyleParameter(parent) {}

ImageParameter::~ImageParameter() = default;

/*!
    \fn void ImageParameter::sourceUpdated()
    \brief Signal emitted when the image source is updated.
*/

/*!
    \brief Get the image source.
    \return image source as \c QString.
*/
QString ImageParameter::source() const {
    return m_source;
}

/*!
    \brief Set the image source.
    \param source image source to set.

    \ref sourceUpdated() signal is emitted when the image source is updated.
*/
void ImageParameter::setSource(const QString &source) {
    if (m_source == source) {
        return;
    }

    m_source = source;

    Q_EMIT sourceUpdated();
}

/*!
    \var ImageParameter::m_source
    \brief image source
*/

} // namespace QMapLibre
