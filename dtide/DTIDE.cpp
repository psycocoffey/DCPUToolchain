#include "DTIDE.h"

DTIDE::DTIDE(Toolchain* t, QString fileName, QWidget* parent): QMainWindow(parent)
{
    menu = menuBar();

    tabs = new DTIDETabWidget(this);
    tabs->setMovable(true);
    setCentralWidget(tabs);
   
    setupMenuBar();
    setupActions();
    setupSignals();
    setupDockWidgets();

    resize(QSize(640, 580));

    toolchain = t;
    addCodeTab(fileName);

    debuggingSession = 0;

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(cycleUpdate()));
    timer->start(1);
}

void DTIDE::cycleUpdate()
{
    runCycles(100); // 1ms = 1kHz, 100 * 1kHz = 100kHz
}

void DTIDE::runCycles(int count)
{
    if(debuggingSession != 0)
    {
        if(count == 1)
            this->toolchain->Step();
        else
            for(int i = 0; i < count; i++) 
                this->toolchain->Cycle();

        this->toolchain->SendStatus();

        while(debuggingSession->HasMessages())
        {
            DebuggingMessage m = debuggingSession->GetMessage();
            switch(m.type)
            {
                case StatusType:
                    StatusMessage status = (StatusMessage&) m.value;
                    emit setRegisters(status);
                    break;
            }
        }
    }
}


void DTIDE::addCodeTab(const QString& fileName)
{
    QFont font;
    font.setFamily("Monospace");
    font.setFixedPitch(true);
    font.setPointSize(10);
    
    CodeEditor* editor = new CodeEditor(toolchain, fileName, this);
    connect(editor, SIGNAL(fileNameChanged(QString)), tabs, SLOT(updateTitle(QString)));
    editor->setFont(font);

    tabs->addTab(editor, fileName);
}

void DTIDE::setupActions()
{
    nextTab = new QAction(tr("Next tab"), this);
    nextTab->setShortcut(QKeySequence::NextChild);
    connect(nextTab, SIGNAL(triggered()), tabs, SLOT(goToNextTab()));
    addAction(nextTab);
}

void DTIDE::setupSignals()
{
    connect(this, SIGNAL(fileSave()), tabs, SLOT(fileSave()));
}

void DTIDE::setupMenuBar()
{
    QMenu* file = new QMenu("&File", this);
    menu->addMenu(file);

    file->addAction("&New file", this, SLOT(newFile()), tr("Ctrl+N"));
    file->addAction("&Open file", this, SLOT(openFile()), tr("Ctrl+O"));
    file->addAction("&Save file", this, SLOT(saveFile()), tr("Ctrl+S"));

    QMenu* project = new QMenu("&Project", this);
    menu->addMenu(project);

    project->addAction("Compil&e", this, SLOT(compileProject()), tr("Ctrl+E"));
    project->addAction("Compile and &run", this, SLOT(compileAndRunProject()), tr("Ctrl+R"));
}

void DTIDE::setupDockWidgets()
{
    registers = new DTIDERegisters(this);
    connect(this, SIGNAL(setRegisters(StatusMessage)), registers, SLOT(setRegisters(StatusMessage)));
    connect(registers, SIGNAL(step()), this, SLOT(step()));
    connect(registers, SIGNAL(start()), this, SLOT(compileAndRunProject()));
    connect(registers, SIGNAL(pause()), this, SLOT(pause()));

    QDockWidget* registersDockWidget = new QDockWidget(tr("Registers"), this);
    registersDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea |
        Qt::RightDockWidgetArea);

    registersDockWidget->setWidget(registers);
    registersDockWidget->setMinimumWidth(100);
    addDockWidget(Qt::RightDockWidgetArea, registersDockWidget);
}

void DTIDE::step()
{
    runCycles(1);
}

void DTIDE::stop()
{
    toolchain->Stop(debuggingSession);
}

void DTIDE::pause()
{
    toolchain->Pause(debuggingSession);
}

void DTIDE::newFile()
{
//    addCodeTab(DTIDEBackends::getUntitledProperties(type));
}

void DTIDE::openFile()
{
}

void DTIDE::saveFile()
{
    emit fileSave();
}

QSize DTIDE::sizeHint()
{
    return QSize(640, 480);
}

void DTIDE::compileAndRunProject()
{
    debuggingSession = new DTIDEDebuggingSession();

    compileProject();
    for(int i = 0; i < tabs->count(); i++)
    {
    	CodeEditor* w = qobject_cast<CodeEditor*>(tabs->widget(i));
    	w->run(debuggingSession);
    }
}

void DTIDE::compileProject()
{
    for(int i = 0; i < tabs->count(); i++)
    {
    	CodeEditor* w = qobject_cast<CodeEditor*>(tabs->widget(i));
    	w->build();
    }
}


void DTIDE::closeEvent(QCloseEvent* event)
{
    // clean up
}
