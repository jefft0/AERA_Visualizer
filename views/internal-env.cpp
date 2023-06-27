//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
//_/_/
//_/_/ AERA Visualizer
//_/_/ 
//_/_/ Copyright (c) 2018-2023 Jeff Thompson
//_/_/ Copyright (c) 2018-2023 Kristinn R. Thorisson
//_/_/ Copyright (c) 2023 Chloe Schaff
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

#include "internal-env.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QLabel>
#include <QGridLayout>

/* To Do
* - Need display for when nothing's loaded in (supported programs, logo, etc.)
* - Faded balls for past histories (have notes with times attached)
* - Ball going past max time should be detected and start sidescrolling
* - Live update with AERA step functions
* - Function call to swap to darkmode brushes
*/

using namespace std;
using namespace std::chrono;

namespace aera_visualizer {

	InternalEnvView::InternalEnvView(AeraVisualizerWindow* mainWindow)
		: QDockWidget("Internal Environment", mainWindow),
		positionY_(0),
		velocityY_(0),
		forceY_(0)
	{
		QWidget* container = new QWidget();
		container->setObjectName("internal_env_container");

		canvas_ = new EnvCanvas(this);

		QLabel* simulationLabel = new QLabel(this);
		simulationLabel->setText("Simulation");
		simulationLabel->setAlignment(Qt::AlignHCenter);
		simulationLabel->setStyleSheet("QLabel { font-weight: bold; }");

		identifierLabel_ = new QLabel(this);
		identifierLabel_->setText("NONE");
		identifierLabel_->setAlignment(Qt::AlignCenter);
		identifierLabel_->setStyleSheet("QLabel { font-size: 18pt; }");

		firstDataLabel_ = new QLabel(this);
		secondDataLabel_ = new QLabel(this);
		thirdDataLabel_ = new QLabel(this);
		firstDataLabel_->setText("Data1: -");
		secondDataLabel_->setText("Data2: -");
		thirdDataLabel_->setText("Data3: -");
		firstDataLabel_->setStyleSheet("QLabel { font-family: courier; }");
		secondDataLabel_->setStyleSheet("QLabel { font-family: courier; }");
		thirdDataLabel_->setStyleSheet("QLabel { font-family: courier; }");

		// Set up layouts
		QGridLayout* dataLayout = new QGridLayout();
		dataLayout->addWidget(simulationLabel, 0, 0);
		dataLayout->addWidget(identifierLabel_, 1, 0, 2, 1);
		dataLayout->addWidget(firstDataLabel_, 0, 1);
		dataLayout->addWidget(secondDataLabel_, 1, 1);
		dataLayout->addWidget(thirdDataLabel_, 2, 1);


		QVBoxLayout* layout = new QVBoxLayout();
		layout->addWidget(canvas_);
		layout->addLayout(dataLayout);
		container->setLayout(layout);

		setWidget(container);
	}

	void InternalEnvView::setAERA(AERA_interface* aera) {
		// Get the updated information
		aera_ = aera;

		// Refresh the drawing and data output
		refresh();
	}

	void InternalEnvView::refresh() {
		identifier_ = aera_->getMem()->getIdentifier();

		// Use identifier_ to decide what to show
		if (identifier_ == "ball") {
			// Update the label
			identifierLabel_->setText("BALL");

			// Get new values from mem_
			positionY_ = aera_->getMem()->getPositionY();
			velocityY_ = aera_->getMem()->getVelocityY();
			forceY_ = aera_->getMem()->getForceY();

			// Update the labels
			QString text;
			firstDataLabel_->setText(text.sprintf("Position: %07.4f", positionY_));
			secondDataLabel_->setText(text.sprintf("Velocity: %07.4f", velocityY_));
			thirdDataLabel_->setText(text.sprintf("Force:    %07.4f", forceY_));

			// Get current elapsed time
			milliseconds elapsedTime = duration_cast<milliseconds>(aera_->getCurrentTime() - aera_->getStartTime());

			// Update the canvas
			canvas_->setState(identifier_, elapsedTime, positionY_, velocityY_, forceY_);
		}
		else if (identifier_ == "cart-pole") {
			identifierLabel_->setText("CART-POLE");

		}
		else {
			// Show not supported message
		}

		update();
	}

	/* EnvCanvas */
	EnvCanvas::EnvCanvas(QWidget* parent)
	:	QWidget(parent)
	{
		// Set up the brushes
		foregroundBrush_ = QBrush(QColor("#008300"));
		backgroundBrush_ = QBrush(QColor("#7ccbcc"));
		objectBrush_ = QBrush(QColor("#b439b3"));
		fadedObjectBrush_ = QBrush(QColor("#d48ed2"));
		transparentBrush_ = QBrush(QColor(0, 0, 0, 0));
	}

	QSize EnvCanvas::minimumSizeHint() const {
		return QSize(100, 100);
	}

	QSize EnvCanvas::sizeHint() const {
		return QSize(400, 400);
	}

	void EnvCanvas::drawTimestamp(QPainter &painter) {
		// Put together a time stamp
		QString timeString = "t = +" + QString::fromStdString(std::to_string(time_.count()) + " ms");

		// Scale the font size with canvas height
		QFont scaledFont = painter.font();
		scaledFont.setPointSizeF(height() * 0.05);
		painter.setFont(scaledFont);

		// Draw the current time in the upper right corner
		int textWidth = painter.fontMetrics().width(timeString);
		QRect textRect(width() - textWidth - 10, 10, textWidth, painter.fontMetrics().height());
		painter.setPen(QPen(palette().text().color()));
		painter.drawText(textRect, timeString);
	}

	void EnvCanvas::drawHticks(int y, float min, float max, float interval, QPainter& painter) {
		int majorTickHeight = round(height() * 0.05 + painter.fontMetrics().height());
		int minorTickHeight = round(height() * 0.02);
	
		// One pixel is this many units
		float scaleFactor = width() / (max - min);

		// If there's room, subdivide the interval
		float subdividedInterval = interval / (int)scaleFactor;

		for (float i = min; i <= max; i += interval) {
			// Draw minor ticks
			for (float j = i; j < i + interval; j += subdividedInterval) {
				int x = round(j * scaleFactor);
				painter.drawLine(x, y, x, y + minorTickHeight);
			}

			// Draw major ticks
			int x = round(i * scaleFactor);
			painter.drawLine(x, y, x, y + majorTickHeight);

			// Draw the tick label
			QString tick;
			painter.drawText(
				x + round(width() * 0.01), y + painter.fontMetrics().height(),
				tick.sprintf("%.1f", i)
			);			
		}
		
	}

	void EnvCanvas::paintEvent(QPaintEvent* event) {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing, true);

		// Draw the background
		painter.fillRect(QRect(0, 0, width(), height()), palette().base());

		if (identifier_ == "ball"){
			// Draw the background
			int groundHeight = round(height() * 0.75);
			painter.fillRect(QRect(0, 0, width(), groundHeight), backgroundBrush_); // Sky
			painter.fillRect(QRect(0, groundHeight, width(), round(height() * 0.25)), foregroundBrush_); // Ground

			drawTimestamp(painter);
			drawHticks(groundHeight, 0, 100, 10, painter);

			// Compute the position and size of the ball
			int ballDiameter = min(round(width() * 0.15), round(height() * 0.15));
			int x = round((positionY_ / 100) * (width()));
			int y = groundHeight - ballDiameter;
			QRect ballRect(x - round(ballDiameter/2), y, ballDiameter, ballDiameter);

			// Draw the ball
			painter.setBrush(objectBrush_);
			painter.drawEllipse(ballRect);			
		}
		else if (identifier_ == "cart-pole") {
			// Show not implemented
		}
		else if (identifier_ != "") {
			// Show not supported
		}
		else {
			// Show not loaded
		}

		// Draw the border
		painter.setBrush(transparentBrush_);
		painter.drawRect(QRect(0, 0, width(), height()));
		
	}
}
