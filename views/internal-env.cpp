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
//#include "../aera-visualizer-window.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QLabel>
#include <QGridLayout>

/* To Do
* - Need display for when nothing's loaded in (supported programs, logo, etc.)
* - Draw ball in starting position when nothing started
* - Faded balls for past histories (have notes with times attached)
* - Ball going past max time should be detected and start sidescrolling
* - Add y axis ticks
* - Add time in upper right corner
* - Live update with AERA step functions
*/

using namespace std;

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
		identifierLabel_->setText(" - ");
		identifierLabel_->setAlignment(Qt::AlignCenter);
		identifierLabel_->setStyleSheet("QLabel { font-size: 18pt; }");

		positionLabel_ = new QLabel(this);
		velocityLabel_ = new QLabel(this);
		forceLabel_ = new QLabel(this);
		positionLabel_->setText("Position: N/A");
		velocityLabel_->setText("Velocity: N/A");
		forceLabel_->setText("Force: N/A");

		// Set up layouts
		QGridLayout* dataLayout = new QGridLayout();
		dataLayout->addWidget(simulationLabel, 0, 0);
		dataLayout->addWidget(identifierLabel_, 1, 0, 2, 1);
		dataLayout->addWidget(positionLabel_, 0, 1);
		dataLayout->addWidget(velocityLabel_, 1, 1);
		dataLayout->addWidget(forceLabel_, 2, 1);


		QVBoxLayout* layout = new QVBoxLayout();
		layout->addWidget(canvas_);
		layout->addLayout(dataLayout);
		container->setLayout(layout);

		setWidget(container);
	}

	void InternalEnvView::setMem(TestMem<r_exec::LObject, r_exec::MemStatic>* mem) {
		// Get the updated information
		mem_ = mem;
		identifier_ = mem_->getIdentifier();

		// Update the label
		if (identifier_ == "ball")
			identifierLabel_->setText("BALL");
		else if (identifier_ == "cart-pole")
			identifierLabel_->setText("CART-POLE");

		// Refresh the drawing and data output
		refresh();
	}

	void InternalEnvView::refresh() {
		// Get new values from mem_
		positionY_ = mem_->getPositionY();
		velocityY_ = mem_->getVelocityY();
		forceY_ = mem_->getForceY();

		// Update the canvas
		canvas_->setState(identifier_, positionY_, velocityY_, forceY_);

		update();
	}

	/* EnvCanvas */
	EnvCanvas::EnvCanvas(QWidget* parent)
	:	QWidget(parent)
	{
		// Nothing here right now
	}

	QSize EnvCanvas::minimumSizeHint() const {
		return QSize(100, 100);
	}

	QSize EnvCanvas::sizeHint() const {
		return QSize(400, 400);
	}

	void EnvCanvas::paintEvent(QPaintEvent* event) {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing, true);

		int groundHeight = round(height() * 0.75);
		painter.fillRect(QRect(0, 0, width(), groundHeight), palette().base()); // Sky
		painter.fillRect(QRect(0, groundHeight, width(), round(height() * 0.25)), palette().alternateBase()); // Ground

		// Draw the ball
		int ballDiameter = min(round(width() * 0.15), round(height() * 0.15));
		int x = round((positionY_ / 100) * width());
		int y = groundHeight - ballDiameter;

		QRect ballRect(x, y, ballDiameter, ballDiameter);
		//painter.setBrush(palette().text());
		painter.drawEllipse(ballRect);
		//painter.brush)

		// Draw the border
		painter.drawRect(QRect(0, 0, width(), height()));
	}
}
