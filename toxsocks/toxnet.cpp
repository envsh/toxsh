
#include "status.hpp"

// #include "commandhandler.h"

#include "toxnet.h"

ToxNet::ToxNet()
    : QObject(0)
{
    // m_hcmd = new CommandHandler();
    // QObject::connect(m_hcmd, &CommandHandler::commandResponeLine, this, &ToxNet::onCommandResponeLine);
    // QObject::connect(m_hcmd, &CommandHandler::commandResponeFinished, this, &ToxNet::onCommandResponeFinished);
    
    //FIXME: start core in a separate function
    //all connections to `core` should be done after its creation because it registers some types
    core = new Core();

    coreThread = new QThread(this);
    core->moveToThread(coreThread);
    connect(coreThread, &QThread::started, core, &Core::start);

    qRegisterMetaType<Status>("Status");

    connect(core, &Core::connected, this, &ToxNet::onConnected);
    connect(core, &Core::disconnected, this, &ToxNet::onDisconnected);
    connect(core, &Core::friendRequestReceived, this, &ToxNet::onFriendRequestReceived);
    connect(core, &Core::friendAddressGenerated, this, &ToxNet::onFriendAddressGenerated);
    connect(core, &Core::friendMessageReceived, this, &ToxNet::messageReceived);
    connect(core, &Core::friendStatusChanged, this, &ToxNet::onFriendStatusChanged);
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
    connect(core, &Core::failedToRemoveFriend, this, &ToxNet::onFailedToRemoveFriend);
    connect(core, &Core::failedToAddFriend, this, &ToxNet::onFailedToAddFriend);
    connect(core, &Core::messageSentResult, pages, &PagesWidget::messageSentResult);
    connect(core, &Core::actionSentResult, pages, &PagesWidget::actionResult);
    connect(core, &Core::friendStatusMessageChanged, friendsWidget, &FriendsWidget::setStatusMessage);
    connect(core, &Core::friendStatusMessageChanged, pages, &PagesWidget::statusMessageChanged);
    */
    connect(core, &Core::failedToStart, this, &ToxNet::onFailedToStartCore);

    coreThread->start(/*QThread::IdlePriority*/);

    connect(this, &ToxNet::friendRequestAccepted, core, &Core::acceptFriendRequest);

}

ToxNet::~ToxNet()
{

}

void ToxNet::onFriendRequestReceived(const QString& userId, const QString& message)
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


void ToxNet::onConnected()
{
    qDebug()<<"Status::Online";
    // ourUserItem->setStatus(Status::Online);
    core->saveConfiguration();
}

void ToxNet::onDisconnected()
{
    qDebug()<<"Status::Offline";
    // ourUserItem->setStatus(Status::Offline);
}

void ToxNet::onFriendAddressGenerated(QString friendAddress)
{
    qDebug()<<friendAddress;
    core->saveConfiguration();
}

void ToxNet::onFriendStatusChanged(int friendId, Status status)
{
    StatusHelper::Info info;
    qDebug()<<friendId<<StatusHelper::getInfo(status).name;
}

void ToxNet::onFailedToStartCore()
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
void ToxNet::messageReceived(int friendId, const QString& message)
{
    qDebug()<<friendId<<" say:"<<message;
    // core->sendMessage(friendId, QString("You say ?") + message);

    // m_hcmd->onNewCommand(message, friendId);
    emit packetReceived(message.toLatin1(), QString());
}

void ToxNet::onCommandResponeLine(int did, QString oline)
{
    qDebug()<<did<<oline;
    core->sendMessage(did, oline);
}

void ToxNet::onCommandResponeFinished(int did)
{
    core->sendMessage(did, QString("\r\n\r\n"));
}
