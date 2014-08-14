#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QFileDialog>
#include <QTimer>
#include <QFile>
#include <QPainter>
#include <QPaintEvent>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QGroupBox>

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
	QImage img(w, h, QImage::Format_ARGB32);

	convertYUV420_NV21toARGB8888(data, w, h, (quint32*)img.bits());

	return QPixmap::fromImage(img);
}

class VideoInfoDialog : public QDialog
{
	Q_OBJECT

public:
	VideoInfoDialog(QWidget *parent)
		: QDialog(parent)
	{
		QVBoxLayout *vlayout = new QVBoxLayout(this);
		setLayout(vlayout);

		QHBoxLayout *h1 = new QHBoxLayout(this);
		QHBoxLayout *h2 = new QHBoxLayout(this);
		QHBoxLayout *h3 = new QHBoxLayout(this);

		vlayout->addLayout(h1);
		vlayout->addLayout(h2);
		vlayout->addLayout(h3);

		m_fileNameEdit = new QLineEdit();
		h1->addWidget(m_fileNameEdit);
		QPushButton *openButton = new QPushButton("Open");
		h1->addWidget(openButton);
		connect(openButton, SIGNAL(clicked()), SLOT(on_openButton_clicked()));

		h2->addWidget(new QGroupBox("Format"));

		QGroupBox *resGroupBox = new QGroupBox("Resolution");
		QVBoxLayout *gv2 = new QVBoxLayout();
		m_widthEdit = new QLineEdit();
		m_heightEdit = new QLineEdit();
		m_widthEdit->setInputMask("0000");
		m_heightEdit->setInputMask("0000");
		gv2->addWidget(m_widthEdit);
		gv2->addWidget(m_heightEdit);
		resGroupBox->setLayout(gv2);
		h2->addWidget(resGroupBox);

		h2->addWidget(new QGroupBox("FPS"));

		h3->setAlignment(Qt::AlignRight);
		QPushButton *playButton = new QPushButton("Play");
		QPushButton *cancelButton = new QPushButton("Cancel");
		h3->addWidget(playButton);
		h3->addWidget(cancelButton);
		connect(playButton, SIGNAL(clicked()), SLOT(on_playButton_clicked()));
		connect(cancelButton, SIGNAL(clicked()), SLOT(on_cancelButton_clicked()));

		setFixedSize(480, 360);
	}

	QString fileName() const
	{
		return m_fileNameEdit->text();
	}

	QString format() const
	{
		return "yuv420p";
	}

	QSize size() const
	{
		QString w = m_widthEdit->text();
		QString h = m_heightEdit->text();
		return QSize(w.toInt(), h.toInt());
	}

	int fps() const
	{
		return 30;
	}

private slots:
	void on_openButton_clicked()
	{
		QString fileName = QFileDialog::getOpenFileName();
		m_fileNameEdit->setText(fileName);
	}

	void on_playButton_clicked()
	{
		this->done(1);
	}

	void on_cancelButton_clicked()
	{
		this->reject();
	}

private:
	QLineEdit *m_fileNameEdit;
	QLineEdit *m_widthEdit;
	QLineEdit *m_heightEdit;
};

class VideoPlayer : public QLabel
{
	Q_OBJECT

public:
	VideoPlayer(QWidget *parent)
		: QLabel(parent)
	{
		connect(&m_timer, SIGNAL(timeout()), SLOT(on_timeout()));
		setAlignment(Qt::AlignCenter);
		setScaledContents(true);
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
		if(m_yuvFile.isOpen())
		{
			m_yuvFile.close();
		}

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

		resize(640, 480);
	}

	~MainWindow()
	{

	}

private slots:
	void on_toolbar_open()
	{
		VideoInfoDialog dlg(this);
		dlg.exec();
		if(dlg.result() == QDialog::Rejected)
		{
			return;
		}

		m_videoPlayer->setFileName(dlg.fileName());
		m_videoPlayer->setVideoSize(dlg.size());
		m_videoPlayer->setFps(dlg.fps());
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