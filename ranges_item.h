
/****************************************************************************
**
** Copyright (C) 2007-2008 Kevin Clague. All rights reserved.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/****************************************************************************
 *
 * This is the graphical representation of step groups.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/

#ifndef ranges_itemH
#define ranges_itemH

#include <QGraphicsRectItem>
#include "metaitem.h"
#include "where.h"

class Ranges;

class MultiStepRangesBackgroundItem : public QGraphicsRectItem, public MetaItem
{
  Ranges *ranges;
  Meta   *meta;
  bool    positionChanged;
  QPointF position;

public:
  MultiStepRangesBackgroundItem(
    Ranges *_ranges,
    QRectF rect, 
    QGraphicsItem *parent, 
    Meta *_meta);
protected:
  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
  virtual void mouseMoveEvent( QGraphicsSceneMouseEvent *event);
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
};

class Range;

class MultiStepRangeBackgroundItem : public QGraphicsRectItem, public MetaItem
{
public:
    PlacementType   relativeType; // single step, multi step
    PlacementType   parentRelativeType;
    Meta           *meta;
    BackgroundMeta *background;
    BorderMeta     *border;
    PlacementMeta  *placement;
    MarginsMeta    *margin;
    BoolMeta       *perStep;
    AllocMeta      *alloc;
    FreeFormMeta   *freeform;
    FontListMeta   *subModelFont;
    StringListMeta *subModelFontColor;
    Context         context;
    Ranges         *ranges;

    MultiStepRangeBackgroundItem(
      Ranges        *_ranges,
      Range         *_range,
      Meta          *_meta,
      int            _offset_x,
      int            _offset_y,
      QGraphicsItem *parent);

    void addStep(Meta *meta);
  
protected:
    void contextMenuEvent (QGraphicsSceneContextMenuEvent *event);
};

class DividerLine;

class DividerItem : public QGraphicsRectItem, public MetaItem
{
  Context context;
public:
    Meta         *meta;
    SepMeta      *sep;
    PlacementType parentRelativeType;
    DividerLine  *lineItem;
    Where         topOfRanges;
    Where         bottomOfRanges;

    DividerItem() {}
    DividerItem(
      Ranges *_ranges,
      Meta   *_meta,
      Where   _topWhere,
      Where   _bottomWhere,
      int     _offset_x,
      int     _offset_y);
    
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
};
class DividerLine : public QGraphicsLineItem, public MetaItem
{
public:
  DividerLine(DividerItem *parent)
  {
    setParentItem(parent);
  }
protected:
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
};    
  
#endif
