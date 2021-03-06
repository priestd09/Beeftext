﻿/// \file
/// \author 
///
/// \brief Implementation of a delegate for the combo picker items
///  
/// Copyright (c) . All rights reserved.  
/// Licensed under the MIT License. See LICENSE file in the project root for full license information. 


#include "stdafx.h"
#include "ComboPickerItemDelegate.h"
#include "PreferencesManager.h"
#include "../ComboList.h"


namespace {
   qint32 kItemVMargin = 2; ///< The size of the vertical margin of an item.
   qint32 kItemHMargin = 10; ///< The size of the horizontal margin of an item.
}


//**********************************************************************************************************************
/// \return The big font used in item rendering.
//**********************************************************************************************************************
QFont bigFont()
{
   QFont font;
   font.setPointSize(12);
   font.setBold(true);
   font.setItalic(false);
   return font;
}


//**********************************************************************************************************************
/// \return The small font used in item rendering.
//**********************************************************************************************************************
QFont smallFont()
{
   QFont font;
   font.setPointSize(8);
   font.setBold(true);
   font.setItalic(false);
   return font;
   
}


//**********************************************************************************************************************
/// \param[in] painter The painter.
/// \param[in] option The style options.
/// \param[in] index The index of the item to paint.
//**********************************************************************************************************************
void ComboPickerItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
   PreferencesManager const& prefs = PreferencesManager::instance();
   bool const light = ((!prefs.useCustomTheme()) || (ETheme::Light == prefs.theme()));
   QColor const bgColor = light ? QColor(0xff,0xff,0xff) : QColor(0x21, 0x21, 0x21);
   QColor const bgSelectedColor(0x50, 0x8c, 0xc8);
   QColor const bigTextColor = light ? QColor(0x4e, 0x4e, 0x4e) : QColor(0xee, 0xee, 0xee);
   QColor const bigTextSelectedColor(0xff, 0xff,0xff);
   QColor const smallTextColor(0xc8, 0xc8, 0xc8);

   // we draw the background. Color depends on wether the item is selected or not.
   bool const selected = option.state & QStyle::State_Selected ;
   painter->setBrush(selected ? bgSelectedColor : bgColor);
   painter->setPen(Qt::NoPen);
   painter->drawRect(option.rect);

   // we exclude the margin from the rendering rect.
   QRect const rect = option.rect.adjusted(kItemHMargin, kItemVMargin, -kItemHMargin, -kItemVMargin);
   
   // First line (combo name) use big font.
   QFont const bFont = bigFont();
   QFontMetrics const bMetrics(bFont);
   // second line (combo keyword) use small font.
   QFont const sFont = smallFont();

   painter->setFont(bFont);
   painter->setPen(selected ? bigTextSelectedColor : bigTextColor);
   painter->drawText(QPoint(rect.left(), rect.top() + bMetrics.ascent()), 
      bMetrics.elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, rect.width()));
   painter->setPen(smallTextColor);
   painter->setFont(sFont);
   painter->drawText(QPoint(rect.left(), rect.bottom() - QFontMetrics(sFont).descent()), 
      index.data(ComboList::KeywordRole).toString());
}


//**********************************************************************************************************************
/// \param[in] option The style options.
//**********************************************************************************************************************
QSize ComboPickerItemDelegate::sizeHint(const QStyleOptionViewItem& option, QModelIndex const&) const
{
   // we compute the size hint for the item. width is trivial, but height needs attention. 
   // An item contains one line of text in big font, one line of text with small font, and there are margins.
   return QSize(option.rect.width(), QFontMetrics(bigFont()).height() + QFontMetrics(smallFont()).height()
      + 2 * kItemVMargin);
}


