
#include </usr/include/x86_64-linux-gnu/qt5/QtWidgets/QWidget>
#include </usr/include/x86_64-linux-gnu/qt5/QtWidgets/QLabel>
#include </usr/include/x86_64-linux-gnu/qt5/QtWidgets/QDesktopWidget>
#include </usr/include/x86_64-linux-gnu/qt5/QtWidgets/QApplication>
#include </usr/include/x86_64-linux-gnu/qt5/QtWidgets/QPushButton>




class Screen_Qt : public QWidget
{
    
Q_OBJECT
    
public:
    QPushButton          _buttonCtrlAltDel;
    QPushButton          _buttonRefresh;
    QPushButton          _buttonDisconnexion;
    const int            _width;
    const int            _height;
    const int            _buttonHeight;
    
    
    Screen_Qt ()
    : QWidget()
    , _buttonCtrlAltDel("CTRL + ALT + DELETE", this)
    , _buttonRefresh("Refresh", this)
    , _buttonDisconnexion("Disconnexion", this)
    , _width(400)
    , _height(300)
    , _buttonHeight(20)
    {
        this->setFixedSize(this->_width, this->_height + this->_buttonHeight);
        this->setMouseTracking(true);
        this->installEventFilter(this);
        this->setAttribute(Qt::WA_DeleteOnClose);
        std::string title = "Desktop from .";
        this->setWindowTitle(QString(title.c_str())); 
    
        QRect rectCtrlAltDel(QPoint(0, this->_height+1),QSize(this->_width/3, this->_buttonHeight));
        this->_buttonCtrlAltDel.setToolTip(this->_buttonCtrlAltDel.text());
        this->_buttonCtrlAltDel.setGeometry(rectCtrlAltDel);
        this->_buttonCtrlAltDel.setCursor(Qt::PointingHandCursor);

        this->_buttonCtrlAltDel.setFocusPolicy(Qt::NoFocus);

        QRect rectRefresh(QPoint(this->_width/3, this->_height+1),QSize(this->_width/3, this->_buttonHeight));
        this->_buttonRefresh.setToolTip(this->_buttonRefresh.text());
        this->_buttonRefresh.setGeometry(rectRefresh);
        this->_buttonRefresh.setCursor(Qt::PointingHandCursor);

        this->_buttonRefresh.setFocusPolicy(Qt::NoFocus);
        
        QRect rectDisconnexion(QPoint(((this->_width/3)*2), this->_height+1),QSize(this->_width-((this->_width/3)*2), this->_buttonHeight));
        this->_buttonDisconnexion.setToolTip(this->_buttonDisconnexion.text());
        this->_buttonDisconnexion.setGeometry(rectDisconnexion);
        this->_buttonDisconnexion.setCursor(Qt::PointingHandCursor);

        this->_buttonDisconnexion.setFocusPolicy(Qt::NoFocus);
        
        this->setFocusPolicy(Qt::StrongFocus); 
        
        QDesktopWidget* desktop = QApplication::desktop();
        int centerW = (desktop->width()/2)  - (this->_width/2);
        int centerH = (desktop->height()/2) - ((this->_height+20)/2);
        this->move(centerW, centerH);
        
        this->show();
    }
    
    ~Screen_Qt() {}
};



int main(int argc, char** argv){
    
    QApplication app(argc, argv);

    Screen_Qt front();
    
    app.exec();
  
}