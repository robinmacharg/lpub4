
/****************************************************************************
**
** Copyright (C) 2007-2009 Kevin Clague. All rights reserved.
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
 * This class implements part list images.  It gathers and maintains a list
 * of part/colors that need to be displayed.  It provides mechanisms for
 * rendering the parts.  It provides methods for organizing the parts into
 * a reasonable looking box for display in your building instructions.
 *
 * Please see lpub.h for an overall description of how the files in LPub
 * make up the LPub program.
 *
 ***************************************************************************/
#include <QMenu>
#include <QGraphicsSceneContextMenuEvent>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include "pli.h"
#include "step.h"
#include "ranges.h"
#include "callout.h"
#include "resolution.h"
#include "render.h"
#include "paths.h"
#include "partslist.h"
#include "ldrawfiles.h"
#include "placementdialog.h"
#include "metaitem.h"
#include "color.h"
#include "partslist.h"
#include "lpub.h"
#include "commonmenus.h"
#include "lpub_preferences.h"
#include "ranges_element.h"
#include "range_element.h"

QCache<QString,QString> Pli::orientation;
    
const Where &Pli::topOfStep()
{
  if (bom || step == NULL || steps == NULL) {
    return gui->topOfPage();
  } else {
    if (step) {
      return step->topOfStep();
    } else {
      return steps->topOfSteps();
    }
  }
}
const Where &Pli::bottomOfStep()
{
  if (bom || step == NULL || steps == NULL) {
    return gui->bottomOfPage();
  } else {
    if (step) {
      return step->bottomOfStep();
    } else {
      return steps->bottomOfSteps();
    }
  }
}
const Where &Pli::topOfSteps()
{
  return steps->topOfSteps();
}
const Where &Pli::bottomOfSteps()
{
  return steps->bottomOfSteps();
}
const Where &Pli::topOfCallout()
{
  return step->callout()->topOfCallout();
}
const Where &Pli::bottomOfCallout()
{
  return step->callout()->bottomOfCallout();
}

/****************************************************************************
 * Part List Images routines
 ***************************************************************************/

PliPart::~PliPart()
{
  instances.clear();
  leftEdge.clear();
  rightEdge.clear();
}

float PliPart::maxMargin()
{
  float margin1 = qMax(instanceMeta.margin.valuePixels(XX),
                       csiMargin.valuePixels(XX));
  if (annotWidth) {
    float margin2 = annotateMeta.margin.valuePixels(XX);
    margin1 = qMax(margin1,margin2);
  }
  return margin1;
}

QString Pli::partLine(QString &line, Where &here, Meta & /*meta*/)
{
  return line + QString(";%1;%2") .arg(here.modelName) .arg(here.lineNumber);
}

void Pli::setParts(
  QStringList &csiParts,
  Meta        &meta,
  bool         _bom)
{
  bom = _bom;
  pliMeta = _bom ? meta.LPub.bom : meta.LPub.pli;

  const int numParts = csiParts.size();
  for (int i = 0; i < numParts; i++) {
    QString part = csiParts[i];
    QStringList sections = part.split(";");
    QString line = sections[0];
    Where here(sections[1],sections[2].toInt());

    QStringList tokens;

    split(line,tokens);

    if (tokens.size() == 15 && tokens[0] == "1") {
      QString &color = tokens[1];
      QString &type = tokens[14];

      QFileInfo info(type);

      QString key = info.baseName() + "_" + color;

      if ( ! parts.contains(key)) {
        PliPart *part = new PliPart(type,color);
        part->annotateMeta = pliMeta.annotate;
        part->instanceMeta = pliMeta.instance;
        part->csiMargin    = pliMeta.part.margin;
        parts.insert(key,part);
      }

      parts[key]->instances.append(here);
    }
  }
}

void Pli::clear()
{
  parts.clear();
}

QHash<int, QString>     annotationString;
QList<QString>          titles;

bool Pli::initAnnotationString()
{
  if (annotationString.empty()) {
    annotationString[1] = "B";  // blue
    annotationString[2] = "G";  // green
    annotationString[3] = "DC"; // dark cyan
    annotationString[4] = "R";  // red
    annotationString[5] = "M";  // magenta
    annotationString[6] = "Br"; // brown
    annotationString[9] = "LB"; // light blue
    annotationString[10]= "LG"; // light green
    annotationString[11]= "C";  // cyan
    annotationString[12]= "LR"; // cyan
    annotationString[13]= "P";  // pink
    annotationString[14]= "Y";  // yellow
    annotationString[22]= "Ppl";// purple
    annotationString[25]= "O";  // orange
  
    annotationString[32+1] = "TB";  // blue
    annotationString[32+2] = "TG";  // green
    annotationString[32+3] = "TDC"; // dark cyan
    annotationString[32+4] = "TR";  // red
    annotationString[32+5] = "TM";  // magenta
    annotationString[32+6] = "TBr"; // brown
    annotationString[32+9] = "TLB"; // light blue
    annotationString[32+10]= "TLG"; // light green
    annotationString[32+11]= "TC";  // cyan
    annotationString[32+12]= "TLR"; // cyan
    annotationString[32+13]= "TP";  // pink
    annotationString[32+14]= "TY";  // yellow
    annotationString[32+22]= "TPpl";// purple
    annotationString[32+25]= "TO";  // orange
    titles << "^Technic Axle\\s+(\\d+)\\s*.*$";
    titles << "^Technic Axle Flexible\\s+(\\d+)\\s*$";
    titles << "^Technic Beam\\s+(\\d+)\\s*$";
    titles << "^Electric Cable RCX\\s+([0-9].*)$";

    // additions by Danny
    titles << "^Electric Mindstorms NXT Cable\\s+([0-9].*)$";
    titles << "^Electric Mindstorms EV3 Cable\\s+([0-9].*)$";

     // VEX parts annotations added by Danny on 14 April 2014
    titles <<"^VEX Beam  1 x\\s+(\\d+)\\s*$";
    titles <<"^VEX Beam  2 x\\s+(\\d+)\\s*$";
    titles <<"^VEX Pin Standoff\\s+([0-9]*\\.?[0-9]*)\\sM$";

    //titles <<"^VEX Beam .*(?=Bent).*(?=(\\d\\d))(\\d+)";
    //titles <<"^VEX Beam .*(?=Bent).*(?=(\\d\\d))(?!90)(\\d+)";
    //titles <<"^VEX Beam.*(?:Double Bent).*(?!90)(\\d\\d)$";
    //titles <<"^VEX Beam .*(?=Bent).*(?!90)(\\d\\d)";
    titles <<"^VEX Beam(?:\\s)(?:(?!Double Bent).)*(?!90)(\\d\\d)$";

    titles <<"^VEX Plate  4 x\\s+(\\d+)\\s*$";
    titles << "^VEX Axle\\s+(\\d+)\\s*.*$";
    titles << "^VEX-2 Smart Cable\\s+([0-9].*)$";
    titles <<"^VEX-2 Rubber Belt\\s+([0-9].*)Diameter";

  }
  return true;
}

void Pli::getAnnotate(
  QString &type,
  QString &annotateStr)
{
  annotateStr.clear();
  if (titles.size() == 0) {
    return;
  }

  annotateStr = PartsList::title(type.toLower());

  // pick up LSynth lengths

  QString title;
  for (int i = 0; i < titles.size(); i++) {
    title = titles[i];
    QRegExp rx(title);
    if (annotateStr.contains(rx)) {
      annotateStr = rx.cap(1);
      return;
    }
  }

  annotateStr.clear();
  return;
}

QString Pli::orient(QString &color, QString type)
{
  type = type.toLower();

  float a = 1, b = 0, c = 0;
  float d = 0, e = 1, f = 0;
  float g = 0, h = 0, i = 1;

  QString *cached = orientation[type];

  if ( ! cached) {
    QString name(Preferences::pliFile);
    QFile file(name);
  
    if (file.open(QFile::ReadOnly | QFile::Text)) {      
      QTextStream in(&file);
  
      while ( ! in.atEnd()) {
        QString line = in.readLine(0);
        QStringList tokens;
 
        split(line,tokens);

        if (tokens.size() != 15) {
          continue;
        }

        QString token14 = tokens[14].toLower();

        if (tokens.size() == 15 && tokens[0] == "1" && token14 == type) {
          cached = new QString(line);
          orientation.insert(type,cached);
          break;
        }
      }
      file.close();
    }
  }

  if (cached) {
    QStringList tokens;

    split(*cached, tokens);

    if (tokens.size() == 15 && tokens[0] == "1") {
      a = tokens[5].toFloat();
      b = tokens[6].toFloat();
      c = tokens[7].toFloat();
      d = tokens[8].toFloat();
      e = tokens[9].toFloat();
      f = tokens[10].toFloat();
      g = tokens[11].toFloat();
      h = tokens[12].toFloat();
      i = tokens[13].toFloat();
    }
  }

  return QString ("1 %1 0 0 0 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11")
    .arg(color)
    .arg(a) .arg(b) .arg(c)
    .arg(d) .arg(e) .arg(f)
    .arg(g) .arg(h) .arg(i)
    .arg(type);
}

int Pli::createPartImage(
  QString  &partialKey,
  QString  &type,
  QString  &color,
  QPixmap  *pixmap)
{
  float modelScale = pliMeta.modelScale.value();
  QString        unitsName = resolutionType() ? "DPI" : "DPCM";

  QString key = QString("%1_%2_%3_%4_%5_%6_%7")
                    .arg(partialKey) 
                    .arg(meta->LPub.page.size.valuePixels(0)) 
                    .arg(resolution())
                    .arg(resolutionType() == DPI ? "DPI" : "DPCM")
                    .arg(modelScale)
                    .arg(pliMeta.angle.value(0))
                    .arg(pliMeta.angle.value(1));
  QString imageName = QDir::currentPath() + "/" +
                      Paths::partsDir + "/" + key + ".png";
  QString ldrName = QDir::currentPath() + "/" + 
                    Paths::tmpDir + "/pli.ldr";
  QFile part(imageName);

  if ( ! part.exists()) {

    // create a temporary DAT to feed to LDGLite
  
    part.setFileName(ldrName);
  
    if ( ! part.open(QIODevice::WriteOnly)) {
      QMessageBox::critical(NULL,QMessageBox::tr(LPUB),
                         QMessageBox::tr("Cannot open file for writing %1:\n%2.")
                         .arg(ldrName)
                         .arg(part.errorString()));
      return -1;
    }
  
    QTextStream out(&part);
    out << orient(color, type);
    part.close();
      
    // feed DAT to LDGLite
  
    int rc = renderer->renderPli(ldrName,imageName,*meta, bom);
  
    if (rc != 0) {
      QMessageBox::warning(NULL,QMessageBox::tr(LPUB),
                         QMessageBox::tr("Render failed for %1 %2\n")
                         .arg(imageName)
                         .arg(Paths::tmpDir+"/part.dat"));
      return -1;
    }
  } 
  pixmap->load(imageName);
  return 0;
}

void Pli::partClass(
  QString &type,
  QString &pclass)
{
  pclass = PartsList::title(type);

  if (pclass.length()) {
    QRegExp rx("^(\\w+)\\s+([0-9a-zA-Z]+).*$");
    if (pclass.contains(rx)) {
      pclass = rx.cap(1);
      if (rx.captureCount() == 2 && rx.cap(1) == "Technic") {
        pclass += rx.cap(2);
      }
    } else {
      pclass = "ZZZ";
    }
  } else {
    pclass = "ZZZ";
  }
}

int Pli::placePli(
  QList<QString> &keys,
  int    xConstraint,
  int    yConstraint,
  bool   packSubs,
  bool   sortType,
  int   &cols,
  int   &width,
  int   &height)
{
  
  // Place the first row
  BorderData borderData;
  borderData = pliMeta.border.valuePixels();
  int left = 0;
  int nPlaced = 0;
  int tallest = 0;
  int topMargin = int(borderData.margin[1]+borderData.thickness);
  int botMargin = topMargin;

  cols = 0;

  width = 0;
  height = 0;

  QString key;

  for (int i = 0; i < keys.size(); i++) {
    parts[keys[i]]->placed = false;
    if (parts[keys[i]]->height > yConstraint) {
      yConstraint = parts[keys[i]]->height;
      // return -2;
    }
  }

  QList< QPair<int, int> > margins;

  while (nPlaced < keys.size()) {

    int i;
    PliPart *part = NULL;

    for (i = 0; i < keys.size(); i++) {
      QString key = keys[i];
      part = parts[key];
      if ( ! part->placed && left + part->width < xConstraint) {
        break;
      }
    }

    if (i == keys.size()) {
      return -1;
    }

    /* Start new col */

    PliPart *prevPart = parts[keys[i]];

    cols++;

    int width = prevPart->width /* + partMarginX */;
    int widest = i;

    prevPart->left = left;
    prevPart->bot  = 0;
    prevPart->placed = true;
    prevPart->col = cols;
    nPlaced++;

    QPair<int, int> margin;

    margin.first = qMax(prevPart->instanceMeta.margin.valuePixels(XX),
                        prevPart->csiMargin.valuePixels(XX));

    tallest = qMax(tallest,prevPart->height);

    int right = left + prevPart->width;
    int bot = prevPart->height;

    botMargin = qMax(botMargin,prevPart->csiMargin.valuePixels(YY));

    // leftEdge is the number of pixels between the left edge of the image
    // and the leftmost pixel in the image

    // rightEdge is the number of pixels between the left edge of the image
    // and the rightmost pixel in the image

    // lets see if any unplaced part fits under the right side
    // of the first part of the column

    bool fits = false;
    for (i = 0; i < keys.size() && ! fits; i++) {
      part = parts[keys[i]];

      if ( ! part->placed) {
        int xMargin = qMax(prevPart->csiMargin.valuePixels(XX),
                           part->csiMargin.valuePixels(XX));
        int yMargin = qMax(prevPart->csiMargin.valuePixels(YY),
                           part->csiMargin.valuePixels(YY));

        // Do they overlap?

        int top;
        for (top = 0; top < part->height; top++) {
          int ltop  = prevPart->height - part->height - yMargin + top;
          if (ltop >= 0 && ltop < prevPart->height) {
            if (prevPart->rightEdge[ltop] + xMargin >
                prevPart->width - part->width + part->leftEdge[top]) {
              break;
            }
          }
        }
        if (top == part->height) {
          fits = true;
          break;
        }
      }
    }
    if (fits) {
      part->left = prevPart->left + prevPart->width - part->width;
      part->bot  = 0;
      part->placed = true;
      part->col = cols;
      nPlaced++;
    }

    // allocate new row

    while (nPlaced < parts.size()) {

      int overlap = 0;

      bool overlapped = false;

      // new possible upstairs neighbors

      for (i = 0; i < keys.size() && ! overlapped; i++) {
        PliPart *part = parts[keys[i]];

        if ( ! part->placed) {

          int splitMargin = qMax(prevPart->topMargin,part->csiMargin.valuePixels(YY));

          // dropping part down into prev part (top part is right edge, bottom left)

          for (overlap = 1; overlap < prevPart->height && ! overlapped; overlap++) {
            if (overlap > part->height) { // in over our heads?

              // slide the part from the left to right until it bumps into previous
              // part
              for (int right = 0, left = 0;
                       right < part->height;
                       right++,left++) {
                if (part->rightEdge[right] + splitMargin > 
                    prevPart->leftEdge[left+overlap-part->height]) {
                  overlapped = true;
                  break;
                }
              }
            } else {
              // slide the part from the left to right until it bumps into previous
              // part
              for (int right = part->height - overlap - 1, left = 0;
                       right < part->height && left < overlap;
                       right++,left++) {
                if (right >= 0 && part->rightEdge[right] + splitMargin > 
                    prevPart->leftEdge[left]) {
                  overlapped = true;
                  break;
                }
              }
            }
          }

          // overlap = 0;

          if (bot + part->height + splitMargin - overlap <= yConstraint) {
            bot += splitMargin;
            break;
          } else {
            overlapped = false;
          }
        }
      }

      if (i == keys.size()) {
        break; // we can't go more Vertical in this column
      }

      PliPart *part = parts[keys[i]];

      margin.first    = part->csiMargin.valuePixels(XX);
      int splitMargin = qMax(prevPart->topMargin,part->csiMargin.valuePixels(YY));

      prevPart = parts[keys[i]];

      prevPart->left = left;
      prevPart->bot  = bot - overlap;
      prevPart->placed = true;
      prevPart->col = cols;
      nPlaced++;

      if (sortType) {
        if (prevPart->width > width) {
          widest = i;
          width = prevPart->width;
        }
      }

      int height = prevPart->height + splitMargin;

      // try to do sub columns

      if (packSubs && 0 /* && overlap == 0 */ ) {
        int subLeft = left + prevPart->width;
        int top = bot + prevPart->height - overlap + prevPart->topMargin;

        // allocate new sub_col

        while (nPlaced < keys.size() && i < parts.size()) {

          PliPart *part = parts[keys[i]];
          int subMargin = 0;
          for (i = 0; i < keys.size(); i++) {
            part = parts[keys[i]];
            if ( ! part->placed) {
              subMargin = qMax(prevPart->csiMargin.valuePixels(XX),part->csiMargin.valuePixels(XX));
              if (subLeft + subMargin + part->width <= right &&
                  bot + part->height + part->topMargin <= top) {
                break;
              }
            }
          }

          if (i == parts.size()) {
            break;
          }

          int width = part->width;
          part->left = subLeft + subMargin;
          part->bot  = bot;
          part->placed = true;
          nPlaced++;

          int subBot = bot + part->height + part->topMargin;

          // try to place sub_row

          while (nPlaced < parts.size()) {

            for (i = 0; i < parts.size(); i++) {
              part = parts[keys[i]];
              subMargin = qMax(prevPart->csiMargin.valuePixels(XX),part->csiMargin.valuePixels(XX));
              if ( ! part->placed &&
                  subBot + part->height + splitMargin <= top &&
                  subLeft + subMargin + part->width <= right) {
                break;
              }
            }

            if (i == parts.size()) {
              break;
            }

            part->left = subLeft + subMargin;
            part->bot  = subBot;
            part->placed = true;
            nPlaced++;

            subBot += part->height + splitMargin;
          }
          subLeft += width;
        }
      }
      
      bot -= overlap;

      // FIMXE:: try to pack something under bottom of the row.

      bot += height;
      if (bot > tallest) {
        tallest = bot;
      }
    }
    topMargin = qMax(topMargin,part->topMargin);

    left += width;

    part = parts[keys[widest]];
    if (part->annotWidth) {
      margin.second = qMax(part->annotateMeta.margin.valuePixels(XX),part->csiMargin.valuePixels(XX));
    } else {
      margin.second = part->csiMargin.valuePixels(XX);
    }
    margins.append(margin);
  }

  width = left;

  int margin;
  int totalCols = margins.size();
  int lastMargin = 0;
  for (int col = 0; col < totalCols; col++) {
    lastMargin = margins[col].second;
    if (col == 0) {
      int bmargin = int(borderData.thickness + borderData.margin[0]);
      margin = qMax(bmargin,margins[col].first);
    } else {
      margin = qMax(margins[col].first,margins[col].second);
    }
    for (int i = 0; i < parts.size(); i++) {
      if (parts[keys[i]]->col >= col+1) {
        parts[keys[i]]->left += margin;
      }
    }
    width += margin;
  }
  if (lastMargin < borderData.margin[0]+borderData.thickness) {
    lastMargin = int(borderData.margin[0]+borderData.thickness);
  }
  width += lastMargin;

  height = tallest;

  for (int i = 0; i < parts.size(); i++) {
    parts[keys[i]]->bot += botMargin;
  }

  height += botMargin + topMargin;

  return 0;
}

void Pli::placeCols(
  QList<QString> &keys)
{
  QList< QPair<int, int> > margins;

  // Place the first row
  BorderData borderData;
  borderData = pliMeta.border.valuePixels();

  float topMargin = parts[keys[0]]->topMargin;
  topMargin = qMax(borderData.margin[1]+borderData.thickness,topMargin);
  float botMargin = parts[keys[0]]->csiMargin.valuePixels(YY);
  botMargin = qMax(borderData.margin[1]+borderData.thickness,botMargin);

  int height = 0;
  int width;

  PliPart *part = parts[keys[0]];

  float borderMargin = borderData.thickness + borderData.margin[XX];

  width = int(qMax(borderMargin, part->maxMargin()));

  for (int i = 0; i < keys.size(); i++) {
    part = parts[keys[i]];
    part->left = width;
    part->bot  = int(botMargin);
    part->col = i;

    width += part->width;

	  if (part->height > height) {
      height = part->height;
	  }

	  if (i < keys.size() - 1) {
	    PliPart *nextPart = parts[keys[i+1]];
      width += int(qMax(part->maxMargin(),nextPart->maxMargin()));
	  }
  }
  part = parts[keys[keys.size()-1]];
  width += int(qMax(part->maxMargin(),borderMargin));
  
  size[0] = width;
  size[1] = int(topMargin + height + botMargin);
}

void Pli::getLeftEdge(
  QImage     &image,
  QList<int> &edge)
{
  QImage alpha = image.alphaChannel();

  for (int y = 0; y < alpha.height(); y++) {
    int x;
    for (x = 0; x < alpha.width(); x++) {
      QColor c = alpha.pixel(x,y);
      if (c.blue()) {
        edge << x;
        break;
      }
    }
    if (x == alpha.width()) {
      edge << x - 1;
    }
  }
}

void Pli::getRightEdge(
  QImage     &image,
  QList<int> &edge)
{
  QImage alpha = image.alphaChannel();

  for (int y = 0; y < alpha.height(); y++) {
    int x;
    for (x = alpha.width() - 1; x >= 0; x--) {
      QColor c = alpha.pixel(x,y);
      if (c.blue()) {
        edge << x;
        break;
      }
    }
    if (x < 0) {
      edge << 0;
    }
  }
}

int Pli::sortPli()
{
  QString key;
  
  widestPart = 0;
  tallestPart = 0;

  foreach(key,parts.keys()) {
    PliPart *part;

    part = parts[key];

    if (PartsList::isKnownPart(part->type) ||
        gui->isUnofficialPart(part->type) ||
        gui->isSubmodel(part->type)) {

      if (part->color == "16") {
        part->color = "0";
      }

      // Part Pixmap

      QFileInfo info(part->type);

      QString imageName = Paths::partsDir + "/" + key + ".png";

      QPixmap *pixmap = new QPixmap();
      
      if (pixmap == NULL) {
        return -1;
      }

      if (createPartImage(key,part->type,part->color,pixmap)) {
        QMessageBox::warning(NULL,QMessageBox::tr("LPub"),
        QMessageBox::tr("Failed to load %1")
        .arg(imageName));
        return -1;
      }

      QImage image = pixmap->toImage();

      part->pixmap = new PGraphicsPixmapItem(this,part,*pixmap,parentRelativeType,part->type, part->color);

      delete pixmap;

      part->pixmapWidth  = image.width(); 
      part->pixmapHeight = image.height();
     
      part->width  = image.width();

      /* Add instance count area */

      QString descr;

      descr = QString("%1x") .arg(part->instances.size(),0,10);
      
      QString font = pliMeta.instance.font.valueFoo();
      QString color = pliMeta.instance.color.value();
      
      part->instanceText = 
        new InstanceTextItem(this,part,descr,font,color,parentRelativeType);

      int textWidth, textHeight;
        
      part->instanceText->size(textWidth,textHeight);

      part->textHeight = textHeight;

      // if text width greater than image width
      // the bounding box is wider

      if (textWidth > part->width) {
        part->width = textWidth;
      }

      /* Add annotation area */

      getAnnotate(part->type,descr);

      if (descr.size()) {

        font = pliMeta.annotate.font.valueFoo();
        color = pliMeta.annotate.color.value();
        part->annotateText =
          new AnnotateTextItem(this,part,descr,font,color,parentRelativeType);

        part->annotateText->size(part->annotWidth,part->annotHeight);

        if (part->annotWidth > part->width) {
          part->width = part->annotWidth;
        }

        part->partTopMargin = part->annotateMeta.margin.valuePixels(YY);

        int hMax = int(part->annotHeight + part->annotateMeta.margin.value(YY));
        for (int h = 0; h < hMax; h++) {
          part->leftEdge  << part->width - part->annotWidth;
          part->rightEdge << part->width;
        }
      } else {
        part->annotateText = NULL;
        part->annotWidth  = 0;
        part->annotHeight = 0;
        part->partTopMargin = 0;
      }
      part->topMargin = part->csiMargin.valuePixels(YY);
      getLeftEdge(image,part->leftEdge);
      getRightEdge(image,part->rightEdge);

      part->partBotMargin = part->instanceMeta.margin.valuePixels(YY);

      /* Lets see if we can slide the text up in the bottom left corner of
       * part image */

      int overlap;
      bool overlapped = false;

      for (overlap = 1; overlap < textHeight && ! overlapped; overlap++) {
        if (part->leftEdge[part->leftEdge.size() - overlap] < textWidth) {
          overlapped = true;
        }
      }

      int hMax = textHeight + part->instanceMeta.margin.valuePixels(YY);
      for (int h = overlap; h < hMax; h++) {
        part->leftEdge << 0;
        part->rightEdge << textWidth;
      }

      part->height = part->leftEdge.size();

      if (bom && pliMeta.sort.value()) {
        QString pclass;
        partClass(part->type,pclass);
        part->sort = QString("%1%2%3%%4")
                     .arg(pclass,80,' ')
                     .arg(part->width, 8,10,QChar('0'))
                     .arg(part->height,8,10,QChar('0'))
                     .arg(part->color);
      } else {
        part->sort = QString("%1%2%3%4")
                     .arg(part->width, 8,10,QChar('0'))
                     .arg(part->height,8,10,QChar('0'))
                     .arg(part->type)
                     .arg(part->color);
      }
      if (part->width > widestPart) {
        widestPart = part->width;
      }
      if (part->height > tallestPart) {
        tallestPart = part->height;
      }
    } else {
      delete parts[key];
      parts.remove(key);
    }
  }

  sortedKeys = parts.keys();

  // We got all the sizes for parts for a given step so sort

  for (int i = 0; i < parts.size() - 1; i++) {
    for (int j = i+1; j < parts.size(); j++) {
      if (parts[sortedKeys[i]]->sort < parts[sortedKeys[j]]->sort) {
        QString t = sortedKeys[i];
        sortedKeys[i] = sortedKeys[j];
        sortedKeys[j] = t;
      }
    }
  }
  
  return 0;
}

int Pli::sizePli(Meta *_meta, PlacementType _parentRelativeType, bool _perStep)
{
  int rc;

  parentRelativeType = _parentRelativeType;
  perStep = _perStep;
  
  if (parts.size() == 0) {
    return 1;
  }

  meta = _meta;

  rc = sortPli();
  if (rc != 0) {
    return rc;
  }
  
  ConstrainData constrainData = pliMeta.constrain.value();
  
  return resizePli(meta,constrainData);
}

int Pli::sizePli(ConstrainData::PliConstrain constrain, unsigned height)
{
  if (parts.size() == 0) {
    return 1;
  }
  
  if (meta) {
    ConstrainData constrainData;
    constrainData.type = constrain;
    constrainData.constraint = height;
  
    return resizePli(meta,constrainData);
  }
  return 1;
}

int Pli::resizePli(
  Meta *meta,
  ConstrainData &constrainData)
{
  
  switch (parentRelativeType) {
    case StepGroupType:
      placement = meta->LPub.multiStep.pli.placement;
    break;
    case CalloutType:
      placement = meta->LPub.callout.pli.placement;
    break;
    default:
      placement = meta->LPub.pli.placement;
    break;
  }

  // Fill the part list image using constraint
  //   Constrain Height
  //   Constrain Width
  //   Constrain Columns
  //   Constrain Area
  //   Constrain Square

  int cols, height;
  bool packSubs = pliMeta.pack.value();
  bool sortType = pliMeta.sort.value();
  int pliWidth,pliHeight;

  if (constrainData.type == ConstrainData::PliConstrainHeight) {
    int cols;
    int rc;
    rc = placePli(sortedKeys,
                  10000000,
                  int(constrainData.constraint),
                  packSubs,
                  sortType,
                  cols,
                  pliWidth,
                  pliHeight);
    if (rc == -2) {
      constrainData.type = ConstrainData::PliConstrainArea;
    }
  } else if (constrainData.type == ConstrainData::PliConstrainColumns) {
	  if (parts.size() <= constrainData.constraint) {
	    placeCols(sortedKeys);
	    pliWidth = Placement::size[0];
	    pliHeight = Placement::size[1];
	    cols = parts.size();
	  } else { 
      int bomCols = int(constrainData.constraint);

      int maxHeight = 0;
      for (int i = 0; i < parts.size(); i++) {
        maxHeight += parts[sortedKeys[i]]->height + parts[sortedKeys[i]]->csiMargin.valuePixels(1);
      }

      maxHeight += maxHeight;

      if (bomCols) {
        for (height = maxHeight/(4*bomCols); height <= maxHeight; height++) {
          int rc = placePli(sortedKeys,10000000,
                            height,
                            packSubs,
                            sortType,
                            cols,
                            pliWidth,
                            pliHeight);
          if (rc == 0 && cols == bomCols) {
            break;
          }
		    }
      }
    }
  } else if (constrainData.type == ConstrainData::PliConstrainWidth) {

    int height = 0;
    for (int i = 0; i < parts.size(); i++) {
      height += parts[sortedKeys[i]]->height;
    }

    int cols;
    int good_height = height;

    for ( ; height > 0; height -= 4) {

      int rc = placePli(sortedKeys,10000000,
                        height,
                        packSubs,
                        sortType,
                        cols,
                        pliWidth,
                        pliHeight);
      if (rc) {
        break;
      }

      int w = 0;

      for (int i = 0; i < parts.size(); i++) {
        int t;
        t = parts[sortedKeys[i]]->left + parts[sortedKeys[i]]->width;
        if (t > w) {
          w = t;
        }
      }
      if (w < constrainData.constraint) {
        good_height = height;
      }
    }
    placePli(sortedKeys,10000000,
             good_height,
             packSubs,
             sortType,
             cols,
             pliWidth,
             pliHeight);
  } else if (constrainData.type == ConstrainData::PliConstrainArea) {

    int height = 0;
    for (int i = 0; i < parts.size(); i++) {
      height += parts[sortedKeys[i]]->height;
    }

    int cols;
    int min_area = height*height;
    int good_height = height;

    // step by 1/10 of inch or centimeter

    int step = int(toPixels(0.1,DPI));

    for ( ; height > 0; height -= step) {

      int rc = placePli(sortedKeys,10000000,
                        height,
                        packSubs,
                        sortType,
                        cols,
                        pliWidth,
                        pliHeight);

      if (rc) {
        break;
      }

      int h = 0;
      int w = 0;

      for (int i = 0; i < parts.size(); i++) {
        int t;
        t = parts[sortedKeys[i]]->bot + parts[sortedKeys[i]]->height;
        if (t > h) {
          h = t;
        }
        t = parts[sortedKeys[i]]->left + parts[sortedKeys[i]]->width;
        if (t > w) {
          w = t;
        }
      }
      if (w*h < min_area) {
        min_area = w*h;
        good_height = height;
      }
    }
    placePli(sortedKeys,10000000,
             good_height,
             packSubs,
             sortType,
             cols,
             pliWidth,
             pliHeight);
  } else if (constrainData.type == ConstrainData::PliConstrainSquare) {

    int height = 0;
    for (int i = 0; i < parts.size(); i++) {
      height += parts[sortedKeys[i]]->height;
    }

    int cols;
    int min_delta = height;
    int good_height = height;
    int step = int(toPixels(0.1,DPI));

    for ( ; height > 0; height -= step) {

      int rc = placePli(sortedKeys,10000000,
                        height,
                        packSubs,
                        sortType,
                        cols,
                        pliWidth,
                        pliHeight);

      if (rc) {
        break;
      }

      int h = pliWidth;
      int w = pliHeight;

      int delta = 0;
      if (w < h) {
        delta = h - w;
      } else if (h < w) {
        delta = w - h;
      }
      if (delta < min_delta) {
        min_delta = delta;
        good_height = height;
      }
    }
    placePli(sortedKeys,10000000,
             good_height,
             packSubs,
             sortType,
             cols,
             pliWidth,
             pliHeight);
  }

  size[0] = pliWidth;
  size[1] = pliHeight;

  return 0;
}

void Pli::positionChildren(
  int height,
  qreal scaleX,
  qreal scaleY)
{
  QString key;

  foreach (key, sortedKeys) {
    PliPart *part = parts[key];
    if (part == NULL) {
      continue;
    }
    float x,y;

    x = part->left;
    y = height - part->bot;

    if (part->annotateText) {
      part->annotateText->setParentItem(background);
      part->annotateText->setPos(
        (x + part->width - part->annotWidth)/scaleX,
        (y - part->height /*+ part->annotHeight*/)/scaleY);
    }

    part->pixmap->setParentItem(background);
    part->pixmap->setPos(
      x/scaleX,
      (y - part->height + part->annotHeight + part->partTopMargin)/scaleY);
    part->pixmap->setTransformationMode(Qt::SmoothTransformation);

    part->instanceText->setParentItem(background);
    part->instanceText->setPos(
      x/scaleX,
      (y - part->textHeight)/scaleY);
  }
}

int Pli::addPli(
  int       submodelLevel,
  QGraphicsItem *parent)
{
  if (parts.size()) {
    background = 
      new PliBackgroundItem(
            this,
            size[0],
            size[1],
            parentRelativeType,
            submodelLevel,
            parent);
          
    if ( ! background) {
      return -1;
    }
  
    background->size[0] = size[0];
    background->size[1] = size[1];

    positionChildren(size[1],1.0,1.0);
  } else {
    background = NULL;
  }
  return 0;
}

void Pli::setPos(float x, float y)
{
  if (background) {
    background->setPos(x,y);
  }
}
void Pli::setFlag(QGraphicsItem::GraphicsItemFlag flag, bool value)
{
  if (background) {
    background->setFlag(flag,value);
  }
}

/*
 * Single step per page                   case 3 top/bottom of step
 * step in step group pli per step = true case 3 top/bottom of step
 * step in callout                        case 3 top/bottom of step
 * step group global pli                  case 2 topOfSteps/bottomOfSteps
 * BOM on single step per page            case 2 topOfSteps/bottomOfSteps
 * BOM on step group page                 case 1
 * BOM on cover page                      case 2 topOfSteps/bottomOfSteps
 * BOM on numbered page
 */

bool Pli::autoRange(Where &top, Where &bottom)
{
  if (bom || ! perStep) {
    top = topOfSteps();
    bottom = bottomOfSteps();
    return steps->list.size() && (perStep || bom);
  } else {
    top = topOfStep();
    bottom = bottomOfStep();
    return false;
  }
}

QString PGraphicsPixmapItem::pliToolTip(
  QString type,
  QString color)
{
  QString title = PartsList::title(type);
  if (title == "") {
    Where here(type,0);
    title = gui->readLine(here);
    title = title.right(title.length() - 2);
  }
  QString toolTip;

  toolTip = LDrawColor::name(color) + " " + type + " \"" + title + "\" - popup menu";
  return toolTip;
}

PliBackgroundItem::PliBackgroundItem(
  Pli           *_pli,
  int            width,
  int            height,
  PlacementType  _parentRelativeType,
  int            submodelLevel, 
  QGraphicsItem *parent)
{
  pli       = _pli;
  grabHeight = height;
  grabber = NULL;

  parentRelativeType = _parentRelativeType;

  QPixmap *pixmap = new QPixmap(width,height);
    
  QString toolTip;

  if (_pli->bom) {
    toolTip = "Bill Of Materials - popup menu";
  } else {
    toolTip = "Part List - popup menu";
  }

  if (parentRelativeType == StepGroupType /* && pli->perStep == false */) {
    if (pli-bom) {
      placement = pli->meta->LPub.bom.placement;
    } else {
      placement = pli->meta->LPub.multiStep.pli.placement;
    }
  } else {
    placement = pli->pliMeta.placement;
  }

  setBackground( pixmap,
                 PartsListType,
                 pli->meta,
                 pli->pliMeta.background,
                 pli->pliMeta.border,
                 pli->pliMeta.margin,
                 pli->pliMeta.subModelColor,
                 submodelLevel,
                 toolTip);

  setPixmap(*pixmap);
  setParentItem(parent);
  if (parentRelativeType != SingleStepType && pli->perStep) {
    setFlag(QGraphicsItem::ItemIsMovable,false);
  }
}

void PliBackgroundItem::placeGrabbers()
{
  QRectF rect = currentRect();
  point = QPointF(rect.left() + rect.width()/2,rect.bottom());
  if (grabber == NULL) {
    grabber = new Grabber(BottomInside,this,myParentItem());
  }
  grabber->setPos(point.x()-grabSize()/2,point.y()-grabSize()/2);
}

void PliBackgroundItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{     
  position = pos();
  positionChanged = false;
  QGraphicsItem::mousePressEvent(event);
  placeGrabbers();
} 
  
void PliBackgroundItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{ 
  positionChanged = true;
  QGraphicsItem::mouseMoveEvent(event);
  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {
    placeGrabbers();
  }   
}

void PliBackgroundItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsItem::mouseReleaseEvent(event);

  if (isSelected() && (flags() & QGraphicsItem::ItemIsMovable)) {

    QPointF newPosition;

    // back annotate the movement of the PLI into the LDraw file.
    newPosition = pos() - position;
    if (newPosition.x() || newPosition.y()) {
      positionChanged = true;
      PlacementData placementData = placement.value();
      placementData.offsets[0] += newPosition.x()/pli->relativeToSize[0];
      placementData.offsets[1] += newPosition.y()/pli->relativeToSize[1];
      placement.setValue(placementData);

      Where here, bottom;
      bool useBot;

      useBot = pli->autoRange(here,bottom);

      if (useBot) {
        here = bottom;
      }
      changePlacementOffset(here,&placement,pli->parentRelativeType);
    }
  }
}

void PliBackgroundItem::contextMenuEvent(
  QGraphicsSceneContextMenuEvent *event)
{
  if (pli) {
    QMenu menu;
      
    QAction *constrainAction  = menu.addAction("Change Shape");
    constrainAction->setWhatsThis(             "Change Shape:\n"
      "  You can change the shape of this parts list.  One way, is\n"
      "  is to ask the computer to make the parts list as small as\n"
      "  possible (area). Another way is to ask the computer to\n"
      "  make it as close to square as possible.  You can also pick\n"
      "  how wide you want it, and the computer will make it as\n"
      "  tall as is needed.  Another way is to pick how tall you\n"
      "  and it, and the computer will make it as wide as it needs.\n"
      "  The last way is to tell the computer how many columns it\n"
      "  can have, and then it will try to make all the columns the\n"
                                               "  same height\n");
                                               
    PlacementData placementData = pli->placement.value();

    QString name = bom ? "Move Bill Of Materials" : "Move Parts List";
    QAction *placementAction  = menu.addAction(name);
    placementAction->setWhatsThis(
      commonMenus.naturalLanguagePlacementWhatsThis(PartsListType,placementData,name));

    QString pl = bom ? "Bill of Materials " : "Parts List ";
    QAction *backgroundAction = commonMenus.backgroundMenu(menu,pl);
    QAction *borderAction     = commonMenus.borderMenu(menu,pl);
    QAction *marginAction     = commonMenus.marginMenu(menu,pl);

    QAction *deleteBomAction  = NULL;

    if (pli->bom) {
      deleteBomAction = menu.addAction("Delete Bill of Materials");
    }

    QAction *selectedAction   = menu.exec(event->screenPos());

    if (selectedAction == NULL) {
      return;
    }
  
    Where top;
    Where bottom;
    bool  local;
      
    Where topOfStep;
    Where bottomOfStep;
    
    if (pli->step) {
      if (pli->bom || pli->perStep) {
        topOfStep = pli->topOfSteps();
        bottomOfStep = pli->bottomOfSteps();
      } else {
        topOfStep = pli->topOfStep();
        bottomOfStep = pli->bottomOfStep();
      }
    } else {
      topOfStep = pli->topOfSteps();
      bottomOfStep = pli->bottomOfSteps();
    }
  
    switch (parentRelativeType) {
      case StepGroupType:
        top    = pli->topOfSteps();
        bottom = pli->bottomOfSteps();
        local = false;
      break;
      case CalloutType:
        top    = pli->topOfCallout();
        bottom = pli->bottomOfCallout();
        local = false;
      break;
      default:
        if (pli->bom || pli->perStep) {
          top    = pli->topOfSteps();
          bottom = pli->bottomOfSteps();
        } else {
          top    = pli->topOfStep();
          bottom = pli->bottomOfStep();
        }
        local = true;
      break;
    }

    QString me = pli->bom ? "BOM" : "PLI";
    if (selectedAction == constrainAction) {
      changeConstraint(me+" Constraint",
                       topOfStep,
                       bottomOfStep,
                       &pli->pliMeta.constrain);
    } else if (selectedAction == placementAction) {
      if (pli->bom) {
        changePlacement(parentRelativeType,
                        pli->perStep,
                        PartsListType,
                        me+" Placement",
                        topOfStep,
                        bottomOfStep,
                       &pli->pliMeta.placement,true,1,0,false);
      } else if (pli->perStep) {
        changePlacement(parentRelativeType,
                        pli->perStep,
                        PartsListType,
                        me+" Placement",
                        pli->topOfStep(),
                        pli->bottomOfStep(),
                       &pli->placement);
      } else {
        changePlacement(parentRelativeType,
                        pli->perStep,
                        PartsListType,
                        me+" Placement",
                        pli->topOfStep(),
                        pli->bottomOfStep(),
                       &pli->placement,true,1,0,false);
      }
    } else if (selectedAction == marginAction) {
      changeMargins(me+" Margins",
                    topOfStep,
                    bottomOfStep,
                    &pli->pliMeta.margin);
    } else if (selectedAction == backgroundAction) {
      changeBackground(me+" Background",
                       top,
                       bottom,
                       &pli->pliMeta.background);
    } else if (selectedAction == borderAction) {
      changeBorder(me+" Border",
                   top,
                   bottom,
                   &pli->pliMeta.border);
    } else if (selectedAction == deleteBomAction) {
      deleteBOM();
    }
  }
}

/*
 * Code for resizing the PLI - part of the resize class described in
 * resize.h
 */

void PliBackgroundItem::resize(QPointF grabbed)
{
  // recalculate corners Y
  
  point = grabbed;

  // Figure out desired height of PLI
  
  if (pli && pli->parentRelativeType == CalloutType) {
    QPointF absPos = pos();
    absPos = mapToScene(absPos);
    grabHeight = int(grabbed.y() - absPos.y());
  } else {
    grabHeight = int(grabbed.y() - pos().y());
  }
    
  ConstrainData constrainData;
  constrainData.type = ConstrainData::PliConstrainHeight;
  constrainData.constraint = grabHeight;

  pli->resizePli(pli->meta, constrainData);
    
  qreal width = pli->size[0];
  qreal height = pli->size[1];
  qreal scaleX = width/size[0];
  qreal scaleY = height/size[1];

  pli->positionChildren(int(height),scaleX,scaleY);
  
  point = QPoint(int(pos().x()+width/2),int(pos().y()+height));
  grabber->setPos(point.x()-grabSize()/2,point.y()-grabSize()/2);

  resetTransform();
  setTransform(QTransform::fromScale(scaleX, scaleY), true);
    
  QList<QGraphicsItem *> kids = childItems();
    
  for (int i = 0; i < kids.size(); i++) {
    kids[i]->resetTransform();
    kids[i]->setTransform(QTransform::fromScale(1.0/scaleX, 1.0/scaleY), true);
  }
    
  sizeChanged = true;
}

void PliBackgroundItem::change()
{
  ConstrainData constrainData;
    
  constrainData.type = ConstrainData::PliConstrainHeight;
  constrainData.constraint = int(grabHeight);
  
  pli->pliMeta.constrain.setValue(constrainData);

  Where top, bottom;
  bool useBot;

  // for single step with BOM, we have to do something special

  useBot = pli->autoRange(top,bottom);
  int append = 1;
  if (pli->bom && pli->steps->relativeType == SingleStepType && pli->steps->list.size() == 1) {
    Range *range = dynamic_cast<Range *>(pli->steps->list[0]);
    if (range->list.size() == 1) {
      append = 0;
    }
  }

  changeConstraint(top,bottom,&pli->pliMeta.constrain,append,useBot);
}

QRectF PliBackgroundItem::currentRect()
{
  if (pli->parentRelativeType == CalloutType) {
    QRectF foo (pos().x(),pos().y(),size[0],size[1]);
    return foo;
  } else {
    return sceneBoundingRect();
  }
}

void AnnotateTextItem::contextMenuEvent(
  QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;
     
  QString pl = "Part Length ";

  QAction *fontAction   = commonMenus.fontMenu(menu,pl);
  QAction *colorAction  = commonMenus.colorMenu(menu,pl);
  QAction *marginAction = commonMenus.marginMenu(menu,pl);

  QAction *selectedAction   = menu.exec(event->screenPos());

  if (selectedAction == NULL) {
    return;
  }
  
  Where top;
  Where bottom;
  
  switch (parentRelativeType) {
    case StepGroupType:
      top    = pli->topOfSteps();
      MetaItem mi;
      mi.scanForward(top,StepGroupMask);
      bottom = pli->bottomOfSteps();
    break;
    case CalloutType:
      top    = pli->topOfCallout();
      bottom = pli->bottomOfCallout();
    break;
    default:
      top    = pli->topOfStep();
      bottom = pli->bottomOfStep();
    break;
  }

  if (selectedAction == fontAction) {
    changeFont(top,bottom,&pli->pliMeta.annotate.font);
  } else if (selectedAction == colorAction) {
    changeColor(top,bottom,&pli->pliMeta.annotate.color);
  } else if (selectedAction == marginAction) {
    changeMargins("Part Length Margins",
                  top,
                  bottom,
                  &pli->pliMeta.annotate.margin);
  }
}

void InstanceTextItem::contextMenuEvent(
  QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;
     
  QString pl = "Parts Count ";

  QAction *fontAction   = commonMenus.fontMenu(menu,pl);
  QAction *colorAction  = commonMenus.colorMenu(menu,pl);
  QAction *marginAction = commonMenus.marginMenu(menu,pl);

  QAction *selectedAction   = menu.exec(event->screenPos());

  if (selectedAction == NULL) {
    return;
  }
  
  Where top;
  Where bottom;
  
  switch (parentRelativeType) {
    case StepGroupType:
      top    = pli->topOfSteps() + 1;
      MetaItem mi;
      mi.scanForward(top,StepGroupMask);
      bottom = pli->bottomOfSteps();
    break;
    case CalloutType:
      top    = pli->topOfCallout();
      bottom = pli->bottomOfCallout();
    break;
    default:
      top    = pli->topOfStep();
      bottom = pli->bottomOfStep();
    break;
  }

  if (selectedAction == fontAction) {
    changeFont(top,bottom,&pli->pliMeta.instance.font,1,false);
  } else if (selectedAction == colorAction) {
    changeColor(top,bottom,&pli->pliMeta.instance.color,1,false);
  } else if (selectedAction == marginAction) {
    changeMargins("Margins",top,bottom,&pli->pliMeta.instance.margin,true,1,false);
  }
}

void PGraphicsPixmapItem::contextMenuEvent(
  QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;
  QString part = "Part ";
  QAction *hideAction = menu.addAction("Hide Part(s) from Parts List");
  QAction *marginAction = commonMenus.marginMenu(menu,part);

#if 0
  QAction *scaleAction  = commonMenus.scaleMenu(menu,part);

  QAction *orientationAction= menu.addAction("Change Part Orientation");
  orientationAction->setDisabled(true);
  orientationAction->setWhatsThis("This doesn't work right now");
#endif

  QAction *selectedAction   = menu.exec(event->screenPos());

  if (selectedAction == NULL) {
    return;
  }

  if (selectedAction == marginAction) {

    changeMargins("Parts List Part Margins",
                  pli->topOfStep(),
                  pli->bottomOfStep(),
                  &pli->pliMeta.part.margin);
  } else if (selectedAction == hideAction) {
    hidePLIParts(this->part->instances);
#if 0
  } else if (selectedAction == scaleAction) {
    changeFloatSpinTop(
      "Parts List",
      "Model Size",
      pli->topOfStep(),
      pli->bottomOfStep(),
      &meta->LPub.pli.modelScale,0);
    gui->clearPLICache();
#endif
  }
}

