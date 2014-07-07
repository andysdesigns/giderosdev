#include <GL/glew.h>

#include "glcanvas.h"
#include "luaapplication.h"
#include "libnetwork.h"


#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif


#include <QMouseEvent>
#include "platform.h"
#include "refptr.h"
#include <stack>
#include <string>
#include <QTimer>
#include <QDesktopServices>
#include <bytebuffer.h>
#include <event.h>
#include <application.h>
#include <QKeyEvent>
#include <QDebug>
#include <keycode.h>

#include <gevent.h>
#include <ginput.h>
#include <ginput-qt.h>

#include <glog.h>

static int __mkdir(const char* path)
{
#ifdef _WIN32
	return _mkdir(path);
#else
	return mkdir(path, 0755);
#endif
}


/*
void PlatformImplementation::openUrl(const char* url)
{
	urls_.push_back(QUrl(url, QUrl::TolerantMode));
}

void PlatformImplementation::openUrls()
{
	for (std::size_t i = 0; i < urls_.size(); ++i)
		QDesktopServices::openUrl(urls_[i]);
	urls_.clear();
}

void PlatformImplementation::memoryWarning()
{
	application_->broadcastMemoryWarning();
}
*/
static Server* g_server = NULL;
static void printToServer(const char* str, int len, void* data)
{
    unsigned int size = 1 + ((len < 0) ? strlen(str) : len) + 1;
	char* buffer = (char*)malloc(size);

	int pos = 0;
	buffer[pos] = 4;
	pos += 1;
	strcpy(buffer + pos, str);
	pos += strlen(str) + 1;
	Q_ASSERT(pos == size);

	g_server->sendData(buffer, size);

	free(buffer);
}

GLCanvas::GLCanvas(QWidget *parent)
	: QGLWidget(parent)
{
	server_ = new Server(15000);
	g_server = server_;
	application_ = new LuaApplication;
	application_->enableExceptions();
	application_->setPrintFunc(printToServer);

	//startTimer(1);

	QTimer* timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));
	timer->start(1);

/*	platformImplementation_ = new PlatformImplementation(application_);
	setPlatformInterface(platformImplementation_);*/

	orientation_ = ePortrait;

	width_ = 320;
	height_ = 480;
	scale_ = 1;

	fps_ = 10000;
	clock_ = iclock();

    //setAccessFileCallback(accessFileCallback_s, this);

	running_ = false;

    playerDaemon_ = new GPlayerDaemon;
}

GLCanvas::~GLCanvas()
{
	if (running_ == true)
	{
        Event event(Event::APPLICATION_EXIT);
        GStatus status;
        application_->broadcastEvent(&event, &status);
        if (status.error())
		{
            errorDialog_.appendString(status.errorString());
			errorDialog_.show();
            printToServer(status.errorString(), -1, NULL);
            printToServer("\n", -1, NULL);
		}
	}

    //setAccessFileCallback(NULL, NULL);

//	application_->broadcastApplicationWillTerminate();
	timerEvent(0);				// TODO: network bufferinda ne kalmissa send etmek icin baska bi fonksiyon yaz

	application_->deinitialize();
	delete application_;

/*	setPlatformInterface(NULL);
	delete platformImplementation_; */

	delete server_;
	g_server = 0;

    delete playerDaemon_;
}

void GLCanvas::initializeGL()
{
    glewInit();

	application_->initialize();
	application_->setHardwareOrientation(orientation_);
	application_->setResolution(width_, height_);
	application_->setScale(scale_);
}

// TODO: renderScene'e full try-catch icine aldigimiz icin. mesela enterFrame event'inde bi exception olursa, 
// geriye kalanlar calistirilmadigi icin ekrana hicbirsey cizilmiyor. renderScene'inin icini parcalamak lazim.
// her logical kismi ayri try-catch icine alabiliriz belki.
// TODO: bu hata olayini iyi dusunmek lazim. bi timer event'inde hata olursa, o timer tekrar cagirilmiyordu. sebebi bu yuzden olsa gerek.
// TODO: belki de lua'yi exception'li derlemek lazim. koda baktigimda oyle birseyi destekliyordu
void GLCanvas::paintGL()
{
    GStatus status;
    application_->enterFrame(&status);
    if (status.error())
	{
		running_ = false;

        errorDialog_.appendString(status.errorString());
		errorDialog_.show();
        printToServer(status.errorString(), -1, NULL);
        printToServer("\n", -1, NULL);
		application_->deinitialize();
		application_->initialize();
	}

	application_->clearBuffers();
	application_->renderScene();

	if (running_ == false)
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glScalef(1.f / scale_, 1.f / scale_, 1);

		void drawInfo();
		drawInfo();
	}

}

void GLCanvas::onTimer()
{
	double deltat = 1.0 / fps_;

/*	if (deltat == 0)
	{
		timerEvent(0);
		return;
	} */

	bool update = false;

	double now = iclock();
	while (now > clock_ + deltat)
	{
		update = true;
		clock_ += deltat;
	}

	if (update)
		timerEvent(0);
}

void GLCanvas::timerEvent(QTimerEvent *)
{
    std::pair<GPlayerDaemon::GCommandType, std::string> command = playerDaemon_->dequeueCommand();

    switch (command.first)
    {
    case GPlayerDaemon::eNone:
        break;
    case GPlayerDaemon::ePlay:
        break;
    case GPlayerDaemon::eStop:
        break;
    }

    update();
}


#if 0

// TODO: TimerEvent.TIMER'da bi exception olursa, o event bir daha cagirilmiyor. Bunun nedeini bulmak lazim
void GLCanvas::timerEvent(QTimerEvent *)
{
//	platformImplementation_->openUrls();
	
//	printf(".");
//	printf("%d\n", Referenced::instanceCount);
	int dataTotal = 0;

	while (true)
	{
		int dataSent0 = server_->dataSent();
		int dataReceived0 = server_->dataReceived();

		NetworkEvent event;
		server_->tick(&event);

//		if (event.eventCode != eNone)
//			printf("%s\n", eventCodeString(event.eventCode));

		int dataSent1 = server_->dataSent();
		int dataReceived1 = server_->dataReceived();

		if (event.eventCode == eDataReceived)
		{
			const std::vector<char>& data = event.data;

			switch (data[0])
			{
			case 0:			// create folder
			{
				std::string folderName = &data[1];
				__mkdir(g_pathForFile(folderName.c_str()));
				break;
			}

			case 1:			// create file
			{
				std::string fileName = &data[1];
				FILE* fos = fopen(g_pathForFile(fileName.c_str()), "wb");
				int pos = 1 + fileName.size() + 1;
				if (data.size() > pos)
					fwrite(&data[pos], data.size() - pos, 1, fos);
				fclose(fos);
				allResourceFiles.insert(fileName);
				calculateMD5(fileName.c_str());
				saveMD5();
				break;
			}
			case 2:
			{
                glog_v("play message is received\n");

				running_ = true;

                //accessedResourceFiles.clear();

				std::vector<std::string> luafiles;

				ByteBuffer buffer(&data[0], data.size());

				char chr;
				buffer >> chr;

				while (buffer.eob() == false)
				{
					std::string str;
					buffer >> str;
					luafiles.push_back(str);
				}

                GStatus status;
                for (std::size_t i = 0; i < luafiles.size(); ++i)
                {
                    application_->loadFile(luafiles[i].c_str(), &status);
                    if (status.error())
                        break;
                }

                if (!status.error())
                {
                    Event event(Event::APPLICATION_START);
                    application_->broadcastEvent(&event, &status);
                }

                if (status.error())
                {
                    running_ = false;

                    errorDialog_.appendString(status.errorString());
                    errorDialog_.show();
                    printToServer(status.errorString(), -1, NULL);
                    printToServer("\n", -1, NULL);
                    application_->deinitialize();
                    application_->initialize();
                }

				break;
			}
			case 3:
			{
                glog_v("stop message is received\n");

				if (running_ == true)
				{
                    Event event(Event::APPLICATION_EXIT);
                    GStatus status;
                    application_->broadcastEvent(&event, &status);

                    if (status.error())
					{
                        errorDialog_.appendString(status.errorString());
						errorDialog_.show();
                        printToServer(status.errorString(), -1, NULL);
                        printToServer("\n", -1, NULL);
					}
				}

				running_ = false;

				application_->deinitialize();
				application_->initialize();
				application_->setHardwareOrientation(orientation_);
				application_->setResolution(width_, height_);
				application_->setScale(scale_);
				break;
			}
			case 5:
			{
				// deleteFiles();
				break;
			}
			case 7:
			{
				sendFileList();
				break;
			}
			case 8:
			{
				ByteBuffer buffer(&data[0], data.size());

				char chr;
				buffer >> chr;
				
				std::string str;
				buffer >> str;

				projectName_ = str.c_str();

				if (projectName_.isEmpty() == false)
				{
					dir_ = QDir::temp();
					dir_.mkdir("gideros");
					dir_.cd("gideros");
					dir_.mkdir(projectName_);
					dir_.cd(projectName_);

					md5filename_ = qPrintable(dir_.absoluteFilePath("md5.txt"));
					loadMD5();

					dir_.mkdir("documents");
					dir_.mkdir("temporary");
					dir_.mkdir("resource");

					resourceDirectory_ = qPrintable(dir_.absoluteFilePath("resource"));

					setDocumentsDirectory(qPrintable(dir_.absoluteFilePath("documents")));
					setTemporaryDirectory(qPrintable(dir_.absoluteFilePath("temporary")));
					setResourceDirectory(resourceDirectory_.c_str());
				}

				emit projectNameChanged(projectName_);
				break;
			}

			case 9:
			{
				ByteBuffer buffer(&data[0], data.size());

				char chr;
				buffer >> chr;

				std::string fileName;
				buffer >> fileName;

				remove(g_pathForFile(fileName.c_str()));

				{
					std::set<std::string>::iterator iter = allResourceFiles.find(fileName);
					if (iter != allResourceFiles.end())		// this if statement is unnecessary, but we put it "ne olur ne olmaz"
						allResourceFiles.erase(iter);
				}

				{
					std::map<std::string, std::vector<unsigned char> >::iterator iter = md5_.find(fileName);
					if (iter != md5_.end())
					{
						md5_.erase(iter);
						saveMD5();
					}
				}

				break;
			}

			case 11:
			{
				ByteBuffer buffer(&data[0], data.size());

				char chr;
				buffer >> chr;

				int scaleMode, logicalWidth, logicalHeight;
				buffer >> scaleMode;
				buffer >> logicalWidth;
				buffer >> logicalHeight;

				application_->deinitialize();
				application_->initialize();
				application_->setResolution(width_, height_);
				application_->setHardwareOrientation(orientation_);
//				application_->orientationChange(orientation_);
				application_->setScale(scale_);
				application_->setLogicalDimensions(logicalWidth, logicalHeight);
				application_->setLogicalScaleMode((LogicalScaleMode)scaleMode);

				int scaleCount;
				buffer >> scaleCount;
				std::vector<std::pair<std::string, float> > imageScales(scaleCount);
				for (int i = 0; i < scaleCount; ++i)
				{
					buffer >> imageScales[i].first;
					buffer >> imageScales[i].second;
				}

				application_->setImageScales(imageScales);

				int orientation;
				buffer >> orientation;
				application_->setOrientation((Orientation)orientation);
                application_->getApplication()->setDeviceOrientation((Orientation)orientation);

				int fps;
				buffer >> fps;

				int retinaDisplay;
				buffer >> retinaDisplay;

				int autorotation;
				buffer >> autorotation;

                int mouseToTouch;
                buffer >> mouseToTouch;
                ginput_setMouseToTouchEnabled(mouseToTouch);

                int touchToMouse;
                buffer >> touchToMouse;
                ginput_setTouchToMouseEnabled(touchToMouse);

                int mouseTouchOrder;
                buffer >> mouseTouchOrder;
                ginput_setMouseTouchOrder(mouseTouchOrder);

				break;
			}

			}
		}

		int dataDelta = (dataSent1 - dataSent0) + (dataReceived1 - dataReceived0);
		dataTotal += dataDelta;

		if (dataDelta == 0 || dataTotal > 1024)
			break;
	}

	update();
}

#endif

void GLCanvas::mousePressEvent(QMouseEvent* event)
{
    ginputp_mouseDown(event->x() * scale_, event->y() * scale_, 0);
}

void GLCanvas::mouseMoveEvent(QMouseEvent* event)
{
    ginputp_mouseMove(event->x() * scale_, event->y() * scale_);
}

void GLCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    ginputp_mouseUp(event->x() * scale_, event->y() * scale_, 0);
}


void GLCanvas::keyPressEvent(QKeyEvent* event)
{
	if (event->isAutoRepeat())
		return;

    ginputp_keyDown(event->key());
}

void GLCanvas::keyReleaseEvent(QKeyEvent* event)
{
	if (event->isAutoRepeat())
		return;

    ginputp_keyUp(event->key());
}


void GLCanvas::setHardwareOrientation(Orientation orientation)
{
	orientation_ = orientation;

	if (application_->isInitialized())
	{
		application_->setHardwareOrientation(orientation_);
//		application_->orientationChange(orientation_);
	}
}

void GLCanvas::setResolution(int width, int height)
{
	width_ = width;
	height_ = height;

	if (application_->isInitialized())
		application_->setResolution(width_, height_);
}


/*
static void getDirectoryListing(const std::string& dir, std::vector<std::string>* files, std::vector<std::string>* directories)
{
	files->clear();
	directories->clear();

	WIN32_FIND_DATAA ffd;
	HANDLE hFind;

	std::string dirstar = dir + "/*";

	hFind = FindFirstFileA(dirstar.c_str(), &ffd);

	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0)
				continue;

		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			directories->push_back(ffd.cFileName);
		else
			files->push_back(ffd.cFileName);
	} while (FindNextFileA(hFind, &ffd) != 0);

	FindClose(hFind);
}
*/

static void deltree(const char* dir)
{
	std::stack<std::string> stack;

	std::string directory = dir;
	char back = directory[directory.size() - 1];
	if (back == '/' || back == '\\')
		directory.resize(directory.size() - 1);

	stack.push(directory);

	while (!stack.empty())
	{
		std::string dir = stack.top();
		stack.pop();

		std::vector<std::string> files, directories;
		getDirectoryListing(dir.c_str(), &files, &directories);

		for (std::size_t i = 0; i < files.size(); ++i)
			remove((dir + "/" + files[i]).c_str());

		for (std::size_t i = 0; i < directories.size(); ++i)
			stack.push(dir + "/" + directories[i]);
	}
}

void GLCanvas::deleteFiles()
{
	deltree(resourceDirectory_.c_str());
//	fileList_.clear();
}

void GLCanvas::setFps(int fps)
{
	fps_ = fps;
	clock_ = iclock();
}

#if 0
void GLCanvas::sendFileList()
{
	allResourceFiles.clear();

	ByteBuffer buffer;

	// type(byte) 6
	// D or F, file (zero ended string), age (int)
	// D or F, file (zero ended string), age (int)
	// ....

	buffer.append((char)6);

	std::vector<std::string> files, directories;
	getDirectoryListingR(resourceDirectory_.c_str(), &files, &directories);

	for (std::size_t i = 0; i < files.size(); ++i)
	{
		buffer.append('F');
		buffer.append(files[i]);
		int age = fileAge(g_pathForFile(files[i].c_str()));
		buffer.append(age);

		std::map<std::string, std::vector<unsigned char> >::iterator iter = md5_.find(files[i]);
		if (iter == md5_.end())
		{
			calculateMD5(files[i].c_str());
			saveMD5();
			iter = md5_.find(files[i]);
		}
		buffer.append(&iter->second[0], 16);


		allResourceFiles.insert(files[i]);
	}

	for (std::size_t i = 0; i < directories.size(); ++i)
	{
		buffer.append('D');
		buffer.append(directories[i]);
	}

	server_->sendData(buffer.data(), buffer.size());
}
#endif


/*
void GLCanvas::accessFileCallback_s(FileType type, const char* filename, void* data)
{
	GLCanvas* that = static_cast<GLCanvas*>(data);
	that->accessFileCallback(type, filename);
}

void GLCanvas::accessFileCallback(FileType type, const char* filename)
{
	if (type == eResourceFile)
		accessedResourceFiles.insert(filename);
}
*/

void GLCanvas::sendRun()
{
	ByteBuffer buffer;
	buffer.append((char)10);
	server_->sendData(buffer.data(), buffer.size());
}

#if 0
void GLCanvas::loadMD5()
{
/*	md5_.clear();

	FILE* fis = fopen(md5filename_.c_str(), "rt");

	if (fis == NULL)
		return;
	
	std::string filename;

	char line[1024];
	int nline = 0;
	while (true)
	{
		line[0] = '\0';
		fgets(line, 1024, fis);

		int len = strlen(line);

		if (len > 0 && line[len - 1] == 0x0a)
		{
			line[len - 1] = '\0';
			len--;
		}

		if (len > 0 && line[len - 1] == 0x0d)
		{
			line[len - 1] = '\0';
			len--;
		}

		if (len == 0)
			break;

		if ((nline % 2) == 0)
			filename = line;
		else
			md5_[filename] = line;

		nline++;
	}

	fclose(fis);
	*/


	md5_.clear();

	FILE* fis = fopen(md5filename_.c_str(), "rb");

	if (fis == NULL)
		return;

	int nfiles;
	fread(&nfiles, sizeof(int), 1, fis);

	for (int i = 0; i < nfiles; ++i)
	{
		int strlen;
		fread(&strlen, sizeof(int), 1, fis);

		char* buffer = (char*)malloc(strlen);
		fread(buffer, 1, strlen, fis);
		std::string filename(buffer, strlen);
		free(buffer);

		std::vector<unsigned char> md5(16);
		fread(&md5[0], 1, 16, fis);

		md5_[filename] = md5;
	}

//	printMD5();
}

void GLCanvas::saveMD5()
{
/*	FILE* fos = fopen(md5filename_.c_str(), "wt");

	if (fos == NULL)
		return;

	std::map<std::string, std::string>::iterator iter, end = md5_.end();
	for (iter = md5_.begin(); iter != end; ++iter)
	{
		fprintf(fos, "%s\n", iter->first.c_str());
		fprintf(fos, "%s\n", iter->second.c_str());
	}
	fclose(fos); */


	FILE* fos = fopen(md5filename_.c_str(), "wb");
	if (fos == NULL)
		return;

	int nfiles = md5_.size();
	fwrite(&nfiles, sizeof(int), 1, fos);

	int i = 0;
	std::map<std::string, std::vector<unsigned char> >::iterator iter, end = md5_.end();
	for (iter = md5_.begin(); iter != end; ++iter, ++i)
	{
		int strlen = iter->first.size();
		fwrite(&strlen, sizeof(int), 1, fos);
		fwrite(iter->first.c_str(), 1, strlen, fos);
		fwrite(&iter->second[0], 1, 16, fos);
	}

	fclose(fos);

//	printMD5();
}

void GLCanvas::calculateMD5(const char* file)
{
	std::vector<unsigned char> md5(16);
	if (md5_fromfile(g_pathForFile(file), &md5[0]))
		md5_[file] = md5;
}


void GLCanvas::printMD5()
{
	std::map<std::string, std::vector<unsigned char> >::iterator iter, end = md5_.end();
	for (iter = md5_.begin(); iter != end; ++iter)
	{
		qDebug() << iter->first.c_str();
		const std::vector<unsigned char>& m = iter->second;

		char buffer[33];
		sprintf(buffer, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
				m[0], m[1], m[2], m[3],
				m[4], m[5], m[6], m[7],
				m[8], m[9], m[10], m[11],
				m[12], m[13], m[14], m[15]);

		qDebug() << buffer;
	}
}

#endif


void GLCanvas::setScale(int scale)
{
	scale_ = scale;

	if (application_->isInitialized())
		application_->setScale(scale_);
}
