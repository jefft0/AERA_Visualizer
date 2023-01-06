//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
//_/_/
//_/_/ AERA Visualizer
//_/_/ 
//_/_/ Copyright (c) 2018-2023 Jeff Thompson
//_/_/ Copyright (c) 2018-2023 Kristinn R. Thorisson
//_/_/ Copyright (c) 2018-2023 Icelandic Institute for Intelligent Machines
//_/_/ http://www.iiim.is
//_/_/
//_/_/ --- Open-Source BSD License, with CADIA Clause v 1.0 ---
//_/_/
//_/_/ Redistribution and use in source and binary forms, with or without
//_/_/ modification, is permitted provided that the following conditions
//_/_/ are met:
//_/_/ - Redistributions of source code must retain the above copyright
//_/_/   and collaboration notice, this list of conditions and the
//_/_/   following disclaimer.
//_/_/ - Redistributions in binary form must reproduce the above copyright
//_/_/   notice, this list of conditions and the following disclaimer 
//_/_/   in the documentation and/or other materials provided with 
//_/_/   the distribution.
//_/_/
//_/_/ - Neither the name of its copyright holders nor the names of its
//_/_/   contributors may be used to endorse or promote products
//_/_/   derived from this software without specific prior 
//_/_/   written permission.
//_/_/   
//_/_/ - CADIA Clause: The license granted in and to the software 
//_/_/   under this agreement is a limited-use license. 
//_/_/   The software may not be used in furtherance of:
//_/_/    (i)   intentionally causing bodily injury or severe emotional 
//_/_/          distress to any person;
//_/_/    (ii)  invading the personal privacy or violating the human 
//_/_/          rights of any person; or
//_/_/    (iii) committing or preparing for any act of war.
//_/_/
//_/_/ THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
//_/_/ CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
//_/_/ INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
//_/_/ MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
//_/_/ DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
//_/_/ CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//_/_/ SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
//_/_/ BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
//_/_/ SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
//_/_/ INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
//_/_/ WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
//_/_/ NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//_/_/ OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
//_/_/ OF SUCH DAMAGE.
//_/_/ 
//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

#include <regex>
#include <QMenu>
#include "../explanation-log-window.hpp"
#include "../aera-visualizer-window.hpp"
#include "../submodules/AERA/r_exec/factory.h"
#include "aera-visualizer-scene.hpp"
#include "promoted-prediction-defeated-item.hpp"

using namespace std;
using namespace core;
using namespace r_code;
using namespace r_exec;

namespace aera_visualizer {

PromotedPredictionDefeatedItem::PromotedPredictionDefeatedItem(
  PromotedSimulatedPredictionDefeatEvent* defeatEvent,
  ReplicodeObjects& replicodeObjects, AeraVisualizerScene* parent)
: AeraGraphicsItem(defeatEvent, replicodeObjects, parent, ""),
  defeatEvent_(defeatEvent)
{
  setBrush(SimulatedItemColor);
  borderNoHighlightPen_ = QPen(Qt::red, 2);

  setMessageHtml();
  setTextItemAndPolygon(messageHtml_, false, SHAPE_STOP);
  setToolTip(toolTipText_);
}

void PromotedPredictionDefeatedItem::setMessageHtml()
{
  expandedMessageHtml_ = "The promoted simulated prediction " + 
    makeHtmlLink(defeatEvent_->promotedFact_, replicodeObjects_) + " was defeated by<br>actual simulated prediction " +
    makeHtmlLink(defeatEvent_->input_, replicodeObjects_) + ". Further predictions disabled.";
  QString messageHtml = "promoted prediction defeated";

  // Set toolTipText_ before adding links and buttons.
  toolTipText_ = htmlify(expandedMessageHtml_, true);
  expandedMessageHtml_ = htmlify("down-pointing-triangle " + expandedMessageHtml_, true);
  expandedMessageHtml_.replace("down-pointing-triangle", "<a href=\"#unexpand\">" + DownPointingTriangleHtml + "</a>");

  messageHtml_ = htmlify("right-pointing-triangle " + messageHtml, true);
  messageHtml_.replace("right-pointing-triangle", "<a href=\"#expand\">" + RightPointingTriangleHtml + "</a>");
}

void PromotedPredictionDefeatedItem::textItemLinkActivated(const QString& link)
{
  if (link == "#expand") {
    setTextItemAndPolygon(expandedMessageHtml_, false, SHAPE_STOP);
    setToolTip("");
    bringToFront();
  }
  else if (link == "#unexpand") {
    setTextItemAndPolygon(messageHtml_, false, SHAPE_STOP);
    setToolTip(toolTipText_);
    bringToFront();
  }
  else
    // For #detail_oid- and others, defer to the base class.
    AeraGraphicsItem::textItemLinkActivated(link);
}

}
