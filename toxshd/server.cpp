
#include "commandhandler.h"

#include "server.h"

Server::Server()
    : QObject(0)
{
    m_hcmd = new CommandHandler();
    QObject::connect(m_hcmd, &CommandHandler::commandResponeLine, this, &Server::onCommandResponeLine);
    QObject::connect(m_hcmd, &CommandHandler::commandResponeFinished, this, &Server::onCommandResponeFinished);
    
    //FIXME: start core in a separate function
    //all connections to `core` should be done after its creation because it registers some types
    core = new Core();

    coreThread = new QThread(this);
    core->moveToThread(coreThread);
    connect(coreThread, &QThread::started, core, &Core::start);

    qRegisterMetaType<Status>("Status");

    connect(core, &Core::connected, this, &Server::onConnected);
    connect(core, &Core::disconnected, this, &Server::onDisconnected);
    connect(core, &Core::friendRequestReceived, this, &Server::onFriendRequestReceived);
    connect(core, &Core::friendMessageReceived, this, &Server::messageReceived);
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

    connect(this, &Server::friendRequestAccepted, core, &Core::acceptFriendRequest);

}

Server::~Server()
{

}

void Server::onFriendRequestReceived(const QString& userId, const QString& message)
{
    qDebug()<<"userid:"<<userId<<",message"<<message;
    emit friendRequestAccepted(userId);
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

////
void Server::messageReceived(int friendId, const QString& message)
{
    qDebug()<<friendId<<" say:"<<message;
    core->sendMessage(friendId, QString("You say ?") + message);

    m_hcmd->onNewCommand(message, friendId);
}

void Server::onCommandResponeLine(int did, QString oline)
{
    core->sendMessage(did, oline);
}

void Server::onCommandResponeFinished(int did)
{
    core->sendMessage(did, QString("\r\n\r\n"));
}
