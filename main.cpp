#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QFileDialog>
#include <QTimer>
#include <QFile>
#include <QPainter>
#include <QPaintEvent>
#include <QLabel>

int convertYUVtoARGB(int y, int u, int v)
{
	int r = y + (int)(1.772f*v);
	int g = y - (int)(0.344f*v + 0.714f*u);
	int b = y + (int)(1.402f*u);
	r = r>255? 255 : r<0 ? 0 : r;
	g = g>255? 255 : g<0 ? 0 : g;
	b = b>255? 255 : b<0 ? 0 : b;
	return 0xff000000 | (r<<16) | (g<<8) | b;
}

void convertYUV420_NV21toARGB8888(const uchar *data, int width, int height, quint32 *pixels)
{
	int size = width*height;
	int u_offset = size;  
	int v_offset = size * 1.25;
	int u, v, y1, y2, y3, y4;

	// i along Y and the final pixels
	// k along pixels U and V
	for(int i=0, k=0; i < size; i+=2, k++) {
		y1 = data[i  ]&0xff;
		y2 = data[i+1]&0xff;
		y3 = data[width+i  ]&0xff;
		y4 = data[width+i+1]&0xff;

		v = data[v_offset + k]&0xff;
		u = data[u_offset + k]&0xff;
		v = v-128;
		u = u-128;

		pixels[i  ] = convertYUVtoARGB(y1, u, v);
		pixels[i+1] = convertYUVtoARGB(y2, u, v);
		pixels[width+i  ] = convertYUVtoARGB(y3, u, v);
		pixels[width+i+1] = convertYUVtoARGB(y4, u, v);

		if (i!=0 && (i+2)%width==0)
			i += width;
	}
}

QPixmap yuvToPixmap(const uchar *data, int w, int h)
{
	QScopedPointer<quint32> p(new quint32[w * h]);

	convertYUV420_NV21toARGB8888(data, w, h, p.data());
	QImage img((uchar*)p.data(), w, h, QImage::Format_ARGB32);
	return QPixmap::fromImage(img);
}

class VideoPlayer : public QLabel
{
	Q_OBJECT

public:
	VideoPlayer(QWidget *parent)
		: QLabel(parent)
	{
		connect(&m_timer, SIGNAL(timeout()), SLOT(on_timeout()));
		setAlignment(Qt::AlignCenter);
	}

	~VideoPlayer()
	{

	}

	void setFileName(const QString &fileName)
	{
		m_fileName = fileName;
	}

	void setVideoSize(const QSize &size)
	{
		m_videoSize = size;
	}

	void setPixelFormat(const QString &pixelFormat)
	{
		m_pixelFormat = pixelFormat;
	}

	void setFps(int fps)
	{
		m_fps = fps;
	}

	void play()
	{
		m_yuvFile.setFileName(m_fileName);
		m_yuvFile.open(QIODevice::ReadOnly);
		m_timer.start(1000 / m_fps);
	}

private slots:
	void on_timeout()
	{
		m_currentFrame = m_yuvFile.read(m_videoSize.width() * m_videoSize.height() * 3 / 2);
		if(m_currentFrame.size() == 0)
		{
			m_yuvFile.seek(0);
			return;
		}

		QPixmap p = yuvToPixmap((uchar*)m_currentFrame.data(), m_videoSize.width(), m_videoSize.height());
		setPixmap(p);
	}

private:
	QByteArray m_currentFrame;
	QFile m_yuvFile;
	QString m_fileName;
	QSize m_videoSize;
	QString m_pixelFormat;
	int m_fps;
	QTimer m_timer;
};

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow()
	{
		QToolBar *tb = addToolBar("Main");
		tb->addAction("open", this, SLOT(on_toolbar_open()));

		m_videoPlayer = new VideoPlayer(this);
		setCentralWidget(m_videoPlayer);
	}

	~MainWindow()
	{

	}

private slots:
	void on_toolbar_open()
	{
		QString fileName = QFileDialog::getOpenFileName(this);
		m_videoPlayer->setFileName(fileName);
		m_videoPlayer->setVideoSize(QSize(960, 540));
		m_videoPlayer->setFps(30);
		m_videoPlayer->play();
	}

private:
	VideoPlayer *m_videoPlayer;
};

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	MainWindow mw;
	mw.show();

	app.exec();
}

#include "main.moc"