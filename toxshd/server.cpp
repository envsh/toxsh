#include "server.h"

Server::Server()
    : QObject(0)
{
    //FIXME: start core in a separate function
    //all connections to `core` should be done after its creation because it registers some types
    core = new Core();

    coreThread = new QThread(this);
    core->moveToThread(coreThread);
    connect(coreThread, &QThread::started, core, &Core::start);

    qRegisterMetaType<Status>("Status");

    connect(core, &Core::connected, this, &Server::onConnected);
    connect(core, &Core::disconnected, this, &Server::onDisconnected);
    connect(core, &Core::friendRequestRecieved, this, &Server::onFriendRequestRecieved);
    /*
    connect(core, SIGNAL(friendStatusChanged(int, Status)), friendsWidget, SLOT(setStatus(int, Status)));
    connect(core, &Core::friendAddressGenerated, ourUserItem, &OurUserItemWidget::setFriendAddress);
    connect(core, &Core::friendAdded, friendsWidget, &FriendsWidget::addFriend);
    connect(core, &Core::friendMessageRecieved, pages, &PagesWidget::messageReceived);
    connect(core, &Core::actionReceived, pages, &PagesWidget::actionReceived);
    connect(core, &Core::friendUsernameChanged, friendsWidget, &FriendsWidget::setUsername);
    connect(core, &Core::friendUsernameChanged, pages, &PagesWidget::usernameChanged);
    connect(core, &Core::friendRemoved, friendsWidget, &FriendsWidget::removeFriend);
    connect(core, &Core::friendRemoved, pages, &PagesWidget::removePage);
    connect(core, &Core::failedToRemoveFriend, this, &Server::onFailedToRemoveFriend);
    connect(core, &Core::failedToAddFriend, this, &Server::onFailedToAddFriend);
    connect(core, &Core::messageSentResult, pages, &PagesWidget::messageSentResult);
    connect(core, &Core::actionSentResult, pages, &PagesWidget::actionResult);
    connect(core, &Core::friendStatusMessageChanged, friendsWidget, &FriendsWidget::setStatusMessage);
    connect(core, &Core::friendStatusMessageChanged, pages, &PagesWidget::statusMessageChanged);
    */
    connect(core, &Core::failedToStart, this, &Server::onFailedToStartCore);

    coreThread->start(/*QThread::IdlePriority*/);
}

Server::~Server()
{

}

void Server::onFriendRequestRecieved(const QString& userId, const QString& message)
{
    qDebug()<<"userid:"<<userId<<",message"<<message;
    /*
    FriendRequestDialog dialog(this, userId, message);

    if (dialog.exec() == QDialog::Accepted) {
        emit friendRequestAccepted(userId);
    }
    */
}


void Server::onConnected()
{
    qDebug()<<"Status::Online";
    // ourUserItem->setStatus(Status::Online);
}

void Server::onDisconnected()
{
    qDebug()<<"Status::Offline";
    // ourUserItem->setStatus(Status::Offline);
}

void Server::onFailedToStartCore()
{
    qDebug()<<"";
    /*
    QMessageBox critical(this);
    critical.setText("If you see this message that means that something very bad has happened.\n\nYou could have reached a limit on how many instances of this program can be run on a single computer, or you could just run out of memory, or something else horrible has happened.\n\nWhichever is the case, the application will terminate after you close this message.");
    critical.setIcon(QMessageBox::Critical);
    critical.exec();
    qApp->quit();
    */
}

