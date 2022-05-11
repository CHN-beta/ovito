////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2022 OVITO GmbH, Germany
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#include <ovito/core/Core.h>
#include "SshConnection.h"

namespace Ovito::Ssh {

/******************************************************************************
* Constructor.
******************************************************************************/
SshConnection::SshConnection(const SshConnectionParameters& serverInfo, QObject* parent) : QObject(parent)
{
    _connectionParams = serverInfo;
    _passwordSet = !serverInfo.password.isEmpty();
    _password = serverInfo.password;
    connect(this, &SshConnection::stateChanged, this, &SshConnection::processStateGuard, Qt::QueuedConnection);

    // Ensure that connections are always properly closed.
    OVITO_ASSERT(QCoreApplication::instance() != nullptr);
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &SshConnection::disconnectFromHost);
}

/******************************************************************************
* Destructor.
******************************************************************************/
SshConnection::~SshConnection()
{
    disconnectFromHost();
    if(_session)
        ::ssh_free(_session);
}

/******************************************************************************
* Opens the connection to the host.
******************************************************************************/
void SshConnection::connectToHost()
{
    if(_state == StateClosed) {
        setState(StateInit, true);
    }
}

/******************************************************************************
* Closes the connection to the host.
******************************************************************************/
void SshConnection::disconnectFromHost()
{
    if(_state != StateClosed && _state != StateClosing && _state != StateCanceledByUser) {

        // Prevent recursion
        setState(StateClosing, false);

        // Close all open channels.
        Q_EMIT doCleanup();

        destroySocketNotifiers();

        if(_session) {
            ::ssh_disconnect(_session);
            ::ssh_free(_session);
            _session = nullptr;
        }

        setState(StateClosed, true);
    }
}

/******************************************************************************
* Cancels the connection.
******************************************************************************/
void SshConnection::cancel()
{
    disconnectFromHost();
    setState(StateCanceledByUser, false);
}

/******************************************************************************
* Sets the internal state variable to a new value.
******************************************************************************/
void SshConnection::setState(State state, bool emitStateChangedSignal)
{
    if(_state != state) {
        ::_ssh_log(SSH_LOG_PROTOCOL, "Ovito::SshConnection::setState()", "state=%i", state);

        _state = state;

        if(_state == StateError)
            destroySocketNotifiers();

        // Emit signals:
        switch(_state) {
        case StateClosed:           Q_EMIT disconnected();            break;
        case StateClosing:                                            break;
        case StateInit:                                               break;
        case StateConnecting:                                         break;
        case StateServerIsKnown:                                      break;
        case StateUnknownHost:      Q_EMIT unknownHost();             break;
        case StateAuthChoose:       Q_EMIT chooseAuth();              break;
        case StateAuthContinue:                                       break;
        case StateAuthNone:                                           break;
        case StateAuthAutoPubkey:                                     break;
        case StateAuthPassword:                                       break;
        case StateAuthNeedPassword: Q_EMIT needPassword();            break;
        case StateAuthKbi:                                            break;
        case StateAuthKbiQuestions: Q_EMIT needKbiAnswers();          break;
        case StateAuthAllFailed:    Q_EMIT allAuthsFailed();          break;
        case StateOpened:           Q_EMIT connected();               break;
        case StateError:            Q_EMIT error();                   break;
        case StateCanceledByUser:   Q_EMIT canceled();                break;
        }
    }

    if(emitStateChangedSignal)
        Q_EMIT stateChanged();
}

/******************************************************************************
* Is called after the state has changed.
******************************************************************************/
void SshConnection::processStateGuard()
{
    if(_processingState)
        return;

    _processingState = true;
    processState();
    _processingState = false;

    if(_writeNotifier && _enableWritableNofifier) {
        enableWritableSocketNotifier();
    }
}

/******************************************************************************
* The main state machine function.
******************************************************************************/
void SshConnection::processState()
{
    switch(_state) {
    case StateClosed:
    case StateClosing:
    case StateUnknownHost:
    case StateAuthChoose:
    case StateAuthNeedPassword:
    case StateAuthKbiQuestions:
    case StateAuthAllFailed:
    case StateError:
    case StateCanceledByUser:
        return;

    case StateInit:
        OVITO_ASSERT(!_session);

        _session = ::ssh_new();
        if(!_session) {
            _errorMessage = tr("Failed to create SSH session object.");
            setState(StateError, false);
            return;
        }
        ::ssh_set_blocking(_session, 0);

        // Enable debug log output if OVITO_SSH_LOG environment variable is set.
        if(!qEnvironmentVariableIsEmpty("OVITO_SSH_LOG")) {
            ::ssh_set_log_level(SSH_LOG_TRACE);
            ::ssh_set_log_callback([](int priority, const char *function, const char *buffer, void *userdata) {
                OVITO_ASSERT(buffer);
                SshConnection* con = static_cast<SshConnection*>(userdata);
                if(con->_lastLogMessage != buffer) {
                    qInfo().noquote().nospace() << "[" 
                        << QTime::currentTime().toString(QStringLiteral("hh:mm:ss.zzz")) << ", " 
                        << priority << "] " 
                        << buffer;
                    con->_lastLogMessage = buffer;
                }
            });
            ::ssh_set_log_userdata(this);
            int verbosity = SSH_LOG_FUNCTIONS;
            setLibsshOption(SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
        }

        // Let user override the list of authentication methods:
        if(!qEnvironmentVariableIsEmpty("OVITO_SSH_AUTHENTICATION_METHODS")) {
            bool ok;
            _useAuths = (UseAuths)qgetenv("OVITO_SSH_AUTHENTICATION_METHODS").toInt(&ok);
            if(!ok || _useAuths & ~(UseAuthNone | UseAuthAutoPubKey | UseAuthPassword | UseAuthKbi)) {
                _errorMessage = tr("Invalid value of environment variable OVITO_SSH_AUTHENTICATION_METHODS.");
                setState(StateError, false);
                return;
            }
            ::_ssh_log(SSH_LOG_PROTOCOL, "Ovito::SshConnection::processState()", "overriding list of acceptable authentication methods: %i", (int)_useAuths);
        }

        // Register authentication callback.
        memset(&_sessionCallbacks, 0, sizeof(_sessionCallbacks));
        _sessionCallbacks.userdata = this;
        _sessionCallbacks.auth_function = &SshConnection::authenticationCallback;
        ssh_callbacks_init(&_sessionCallbacks);
        ::ssh_set_callbacks(_session, &_sessionCallbacks);

        // Activate download stream compression.
        setLibsshOption(SSH_OPTIONS_COMPRESSION_S_C, "yes");

        if((_connectionParams.userName.isEmpty() || setLibsshOption(SSH_OPTIONS_USER, qPrintable(_connectionParams.userName)))
                && setLibsshOption(SSH_OPTIONS_HOST, qPrintable(_connectionParams.host))
                && (_connectionParams.port == 0 || setLibsshOption(SSH_OPTIONS_PORT, &_connectionParams.port)))
        {
            ::ssh_options_parse_config(_session, nullptr);
            setState(StateConnecting, true);
        }
        return;

    case StateConnecting:
        switch(::ssh_connect(_session)) {
        case SSH_AGAIN:
            createSocketNotifiers();
            enableWritableSocketNotifier();
            break;

        case SSH_OK:
            createSocketNotifiers();
            setState(StateServerIsKnown, true);
            break;

        case SSH_ERROR:
            setState(StateError, false);
            break;
        }
        return;

    case StateServerIsKnown:
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
        switch(auto knownState = ::ssh_is_server_known(_session)) {
QT_WARNING_POP
        case SSH_SERVER_ERROR:
            setState(StateError, false);
            return;

        case SSH_SERVER_NOT_KNOWN:
        case SSH_SERVER_KNOWN_CHANGED:
        case SSH_SERVER_FOUND_OTHER:
        case SSH_SERVER_FILE_NOT_FOUND:
            _unknownHostType = static_cast<HostState>(knownState);
            setState(StateUnknownHost, false);
            return;

        case SSH_SERVER_KNOWN_OK:
            _unknownHostType = HostKnown;
            tryNextAuth();
            return;
        }
        return;

    case StateAuthContinue:
        tryNextAuth();
        return;

    case StateAuthNone:
        handleAuthResponse(::ssh_userauth_none(_session, nullptr), UseAuthNone);
        return;

    case StateAuthAutoPubkey:
        handleAuthResponse(::ssh_userauth_autopubkey(_session, nullptr), UseAuthAutoPubKey);
        return;

    case StateAuthPassword:
        if(::ssh_get_status(_session) == SSH_CLOSED || ::ssh_get_status(_session) == SSH_CLOSED_ERROR) {
            setState(StateError, false);
        }
        else if(!_passwordSet) {
            setState(StateAuthNeedPassword, false);
        }
        else {
            QByteArray utf8pw = _password.toUtf8();
            auto rc = ::ssh_userauth_password(_session, nullptr, utf8pw.constData());

            if(rc != SSH_AUTH_AGAIN) {
                _passwordSet = false;
                _password.clear();
            }

            handleAuthResponse(rc, UseAuthPassword);
        }
        return;

    case StateAuthKbi: {
        auto rc = ::ssh_userauth_kbdint(_session, nullptr, nullptr);
        if(rc == SSH_AUTH_INFO) {
            // Sometimes SSH_AUTH_INFO is returned even though there are no
            // KBI questions available, in that case, continue as if
            // SSH_AUTH_AGAIN was returned.
            if(::ssh_userauth_kbdint_getnprompts(_session) <= 0)
                enableWritableSocketNotifier();
            else
                setState(StateAuthKbiQuestions, false);
        }
        else {
            handleAuthResponse(rc, UseAuthKbi);
        }
        } return;

    case StateOpened:
        if(::ssh_get_status(_session) == SSH_CLOSED || ::ssh_get_status(_session) == SSH_CLOSED_ERROR) {
            setState(StateError, false);
        }
        else {
            // Activate processState() function on all children so that they can
            // process their events and read and write IO.
            Q_EMIT doProcessState();
        }
        return;
    }

    Q_ASSERT_X(false, __func__, "Case was not handled properly");
}

/******************************************************************************
* Sets an option of the libssh session object.
******************************************************************************/
bool SshConnection::setLibsshOption(enum ssh_options_e type, const void* value)
{
    OVITO_ASSERT(_session);
    if(_state == StateError)
        return false;
    if(::ssh_options_set(_session, type, value) != 0) {
        setState(StateError, true);
        return false;
    }
    return true;
}

/******************************************************************************
* Creates the notifier objects for the sockets.
******************************************************************************/
void SshConnection::createSocketNotifiers()
{
    if(!_readNotifier) {
        _readNotifier = new QSocketNotifier(::ssh_get_fd(_session), QSocketNotifier::Read, this);
        connect(_readNotifier, &QSocketNotifier::activated, this, &SshConnection::handleSocketReadable);
    }

    if(!_writeNotifier) {
        _writeNotifier = new QSocketNotifier(::ssh_get_fd(_session), QSocketNotifier::Write, this);
        connect(_writeNotifier, &QSocketNotifier::activated, this, &SshConnection::handleSocketWritable);
    }
}

/******************************************************************************
* Destroys the notifier objects for the sockets.
******************************************************************************/
void SshConnection::destroySocketNotifiers()
{
    if(_readNotifier) {
        _readNotifier->disconnect(this);
        _readNotifier->setEnabled(false);
        _readNotifier->deleteLater();
        _readNotifier = nullptr;
    }

    if(_writeNotifier) {
        _writeNotifier->disconnect(this);
        _writeNotifier->setEnabled(false);
        _writeNotifier->deleteLater();
        _writeNotifier = nullptr;
    }
}

/******************************************************************************
* Re-enables the writable socket notifier.
******************************************************************************/
void SshConnection::enableWritableSocketNotifier()
{
    if(_processingState) {
        _enableWritableNofifier = true;
    }
    else if(_writeNotifier) {
        auto status = ::ssh_get_status(_session);
        if(status == SSH_CLOSED_ERROR || status == SSH_CLOSED) {
            setState(StateError, false);
            return;
        }
        _writeNotifier->setEnabled(true);
    }
}

/******************************************************************************
* Handles the signal from the QSocketNotifier.
******************************************************************************/
void SshConnection::handleSocketReadable()
{
    _readNotifier->setEnabled(false);
    processStateGuard();
    if(_readNotifier)
        _readNotifier->setEnabled(true);
}

/******************************************************************************
* Handles the signal from the QSocketNotifier.
******************************************************************************/
void SshConnection::handleSocketWritable()
{
    _enableWritableNofifier = false;
    _writeNotifier->setEnabled(false);
    processStateGuard();
}

/******************************************************************************
* Enable or disable one or more authentications.
******************************************************************************/
void SshConnection::useAuth(UseAuths auths, bool enabled)
{
    if(enabled) {
        _useAuths |= auths;
        if(_state == StateAuthChoose || _state == StateAuthAllFailed) {
            setState(StateAuthContinue, true);
        }
    }
    else {
        _useAuths &= ~auths;
    }
}

/******************************************************************************
* Chooses next authentication method to try.
******************************************************************************/
void SshConnection::tryNextAuth()
{
    ::_ssh_log(SSH_LOG_PROTOCOL, "Ovito::SshConnection::tryNextAuth()", "state=%i", _state);

    // If authentication methods have not been chosen or all chosen authentication
    // methods have failed, switch state to StateChooseAuth or StateAuthFailed,
    // respectively.

    UseAuths failedAuth = UseAuthEmpty;

    // Detect failed authentication methods
    switch(_state) {
    case StateClosed:
    case StateClosing:
    case StateInit:
    case StateConnecting:
    case StateServerIsKnown:
    case StateUnknownHost:
    case StateAuthChoose:
    case StateAuthContinue:
    case StateAuthNeedPassword:
    case StateAuthKbiQuestions:
    case StateAuthAllFailed:
    case StateOpened:
    case StateError:
    case StateCanceledByUser:
        break;

    case StateAuthNone:
        failedAuth = UseAuthNone;

        // Disable authentication methods that are not supported by the server.
        {
            AuthMethods supportedMethods = supportedAuthMethods();
            ::_ssh_log(SSH_LOG_PROTOCOL, "Ovito::SshConnection::tryNextAuth()", "server supportedMethods=%i", (int)supportedMethods);
            if(!supportedMethods.testFlag(AuthMethodPassword)) useAuth(UseAuthPassword, false);
            if(!supportedMethods.testFlag(AuthMethodKbi)) useAuth(UseAuthKbi, false);
            if(!supportedMethods.testFlag(AuthMethodPublicKey)) useAuth(UseAuthAutoPubKey, false);
        }

        break;

    case StateAuthAutoPubkey:
        failedAuth = UseAuthAutoPubKey;
        break;

    case StateAuthPassword:
        failedAuth = UseAuthPassword;
        break;

    case StateAuthKbi:
        failedAuth = UseAuthKbi;
        break;
    }

    if(failedAuth != UseAuthEmpty) {
        _failedAuths |= failedAuth;
        State oldState = _state;
        Q_EMIT authFailed(failedAuth);

        // User might close or otherwise manipulate the SshConnection when an
        // authentification fails, so make sure that the state has not been changed.
        if(_state != oldState)
            return;
    }

    // Choose next state for connection:
    if(_useAuths == UseAuthEmpty && _failedAuths == UseAuthEmpty) {
        setState(StateAuthChoose, false);
    }
    else if(_useAuths == UseAuthEmpty) {
        setState(StateAuthAllFailed, false);
    }
    else if(_useAuths & UseAuthNone) {
        _useAuths &= ~UseAuthNone;
        setState(StateAuthNone, true);
    }
    else if(_useAuths & UseAuthAutoPubKey) {
        _useAuths &= ~UseAuthAutoPubKey;
        setState(StateAuthAutoPubkey, true);
    }
    else if(_useAuths & UseAuthPassword) {
        _useAuths &= ~UseAuthPassword;
        setState(StateAuthPassword, true);
    }
    else if(_useAuths & UseAuthKbi) {
        _useAuths &= ~UseAuthKbi;
        setState(StateAuthKbi, true);
    }
}

/******************************************************************************
* Sets the password for use in password authentication.
******************************************************************************/
void SshConnection::setPassword(QString password)
{
    _passwordSet = true;
    _password = std::move(password);

    if(_state == StateAuthNeedPassword) {
        setState(StateAuthPassword, true);
    }
}

/******************************************************************************
* Handles the server's reponse to an authentication attempt.
******************************************************************************/
void SshConnection::handleAuthResponse(int rc, UseAuthFlag auth)
{
    ::_ssh_log(SSH_LOG_PROTOCOL, "Ovito::SshConnection::handleAuthResponse()", "rc=%i auth=%i", rc, auth);

    switch(rc) {
    case SSH_AUTH_AGAIN:
        enableWritableSocketNotifier();
        return;

    case SSH_AUTH_ERROR:
        setState(StateError, false);
        return;

    case SSH_AUTH_DENIED:
        tryNextAuth();
        return;

    case SSH_AUTH_PARTIAL:
        tryNextAuth();
        return;

    case SSH_AUTH_SUCCESS:
        _succeededAuth = auth;
        setState(StateOpened, true);
        return;
    }

    Q_ASSERT_X(false, __func__, "Case was not handled properly");
}

/******************************************************************************
*  Generates a message string to show to the user why the current host is unknown.
******************************************************************************/
QString SshConnection::unknownHostMessage()
{
    switch(unknownHostType()) {

    case HostKnown:
        return tr("This host is known.");

    case HostUnknown:
    case HostKnownHostsFileMissing:
        return tr("The authenticity of the host can't be established or the host is unknown.");

    case HostKeyChanged:
        return tr(
                "WARNING: The public key sent by this host does not match the "
                "expected value. A third party may be attempting to "
                "impersonate the host.");

    case HostKeyTypeChanged:
        return tr(
                "WARNING: The public key type sent by this host does not "
                "match the expected value. A third party may be attempting "
                "to impersonate the host.");
    }

    Q_ASSERT_X(false, __func__, "Case was not handled properly");
    return {};
}

/******************************************************************************
* Returns MD5 hexadecimal hash of the servers public key.
******************************************************************************/
QString SshConnection::hostPublicKeyHash()
{
    ssh_key key;
#if LIBSSH_VERSION_INT > SSH_VERSION_INT(0, 7, 5)
    if(::ssh_get_server_publickey(_session, &key) != SSH_OK) {
        qWarning() << "Call to ssh_get_server_publickey() failed";
        return {};
    }
#else
    if(::ssh_get_publickey(_session, &key) != SSH_OK) {
        qWarning() << "Call to ssh_get_publickey() failed";
        return {};
    }
#endif

    unsigned char* hash;
    size_t hash_len;
    if(::ssh_get_publickey_hash(key, SSH_PUBLICKEY_HASH_MD5, &hash, &hash_len) < 0) {
        ::ssh_key_free(key);
        return {};
    }

    char* hexa = ::ssh_get_hexa(hash, hash_len);
    QString string(hexa);

    ::ssh_string_free_char(hexa);
    ::ssh_clean_pubkey_hash(&hash);
    ::ssh_key_free(key);

    return string;
}

/******************************************************************************
* This turns the current remote host into a known host by adding it to
* the knows_hosts file.
******************************************************************************/
bool SshConnection::markCurrentHostKnown()
{
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    switch(::ssh_write_knownhost(_session)) {
QT_WARNING_POP
    case SSH_OK:
        setState(StateServerIsKnown, true);
        return true;

    case SSH_ERROR:
        return false;

    default:
        return false;
    }
}

/******************************************************************************
* Returns the error message string after the error() signal was emitted.
******************************************************************************/
QString SshConnection::errorMessage() const
{
    if(!_errorMessage.isEmpty())
        return _errorMessage;
    else if(_session)
        return QString(::ssh_get_error(_session));
    else
        return tr("Could not initialize SSH session.");
}

/******************************************************************************
* Returns the username used to log in to the server.
******************************************************************************/
QString SshConnection::username() const
{
    QString user;
    char* s;
    if(::ssh_options_get(_session, SSH_OPTIONS_USER, &s) == SSH_OK) {
        user = s;
        ::ssh_string_free_char(s);
    }
    return user;
}

/******************************************************************************
* Returns the host this connection is to.
******************************************************************************/
QString SshConnection::hostname() const
{
    QString host;
    char* s;
    if(::ssh_options_get(_session, SSH_OPTIONS_HOST, &s) == SSH_OK) {
        host = s;
        ::ssh_string_free_char(s);
    }
    return host;
}

/******************************************************************************
* This is a callback that gets called by libssh whenever a passphrase is required.
******************************************************************************/
int SshConnection::authenticationCallback(const char* prompt, char* buf, size_t len, int echo, int verify, void* userdata)
{
    SshConnection* connection = static_cast<SshConnection*>(userdata);
    if(!connection)
        return -1;

    connection->_keyPassphrase.clear();
    ::_ssh_log(SSH_LOG_PROTOCOL, "Ovito::SshConnection::authenticationCallback()", "emit signal needPassphrase");
    Q_EMIT connection->needPassphrase(prompt);
    if(!connection->_keyPassphrase.isEmpty()) {
        ::_ssh_log(SSH_LOG_PROTOCOL, "Ovito::SshConnection::authenticationCallback()", "received passphrase from user");
        QByteArray utf8pw = connection->_keyPassphrase.toUtf8();
        qstrncpy(buf, utf8pw.constData(), len);
        return 0;
    }

    return -1;
}

/******************************************************************************
* Gets list of Keyboard Interactive questions sent by the server.
******************************************************************************/
QList<SshConnection::KbiQuestion> SshConnection::kbiQuestions()
{
    OVITO_ASSERT(_state== StateAuthKbiQuestions);
    if(_state == StateAuthKbiQuestions) {

        QList<KbiQuestion> questions;
        QString instruction = ::ssh_userauth_kbdint_getinstruction(_session);

        int len = ::ssh_userauth_kbdint_getnprompts(_session);

        for(int i = 0; i < len; i++) {

            char echo = 0;
            const char *prompt = 0;

            prompt = ::ssh_userauth_kbdint_getprompt(_session, i, &echo);
            OVITO_ASSERT(prompt);

            KbiQuestion kbi_question;
            kbi_question.instruction = instruction;
            kbi_question.question    = QString(prompt);
            kbi_question.showAnswer  = echo != 0;
            questions << kbi_question;
        }

        OVITO_ASSERT(questions.count() != 0);
        return questions;

    }
    else {
        qWarning() << "Cannot get KBI questions because state is" << _state;
        return {};
    }
}

/******************************************************************************
* Sets the answers to Keyboard Interactive questions.
******************************************************************************/
void SshConnection::setKbiAnswers(QStringList answers)
{
    OVITO_ASSERT(_state == StateAuthKbiQuestions);
    if(_state == StateAuthKbiQuestions) {
        int i = 0;
        for(const QString& answer : answers) {
            QByteArray utf8 = answer.toUtf8();
            ::ssh_userauth_kbdint_setanswer(_session, i, utf8.constData());
        }

        setState(StateAuthKbi, true);
    }
    else {
        qWarning() << "Cannot set KBI answers because state is" << _state;
    }
}

} // End of namespace
