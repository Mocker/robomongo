#include "robomongo/gui/dialogs/SSLTab.h"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QRegExpValidator>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QComboBox>
#include <QMessageBox>
#include <QFileInfo>
#include <QFrame>
#include <QRadioButton>
#include <QSpacerItem>
#include <QHBoxLayout>
#include <QGroupbox>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/utils/ComboBoxUtils.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/core/settings/SslSettings.h"

#include "mongo/util/net/ssl_options.h"
#include "mongo/util/net/ssl_manager.h"
//#include "mongo/client/init.h"

namespace {
    const QString askPasswordText = "Ask for password each time";
    const QString askPassphraseText = "Ask for passphrase each time";
    bool isFileExists(const QString &path) {
        QFileInfo fileInfo(path);
        return fileInfo.exists() && fileInfo.isFile();
    }
}


namespace Robomongo
{
    SSLTab::SSLTab(ConnectionSettings *settings) 
        : _settings(settings)
    {
        SslSettings *sslSettings = settings->sslSettings();

        // Use SSL section
        _useSslCheckBox = new QCheckBox("Use SSL protocol");
        _useSslCheckBox->setStyleSheet("margin-bottom: 7px");
        _useSslCheckBox->setChecked(sslSettings->enabled());
        VERIFY(connect(_useSslCheckBox, SIGNAL(stateChanged(int)), this, SLOT(useSslCheckBoxStateChange(int))));

        // CA Section
        _acceptSelfSignedButton = new QRadioButton("Accept self-signed certificates");
        _useRootCaFileButton = new QRadioButton("Use Root CA file: ");
        _acceptSelfSignedButton->setChecked(sslSettings->allowInvalidCertificates());
        _useRootCaFileButton->setChecked(!_acceptSelfSignedButton->isChecked());
        //_caFileLabel = new QLabel("Root CA file: ");
        _caFilePathLineEdit = new QLineEdit;
        _caFileBrowseButton = new QPushButton("...");
        _caFileBrowseButton->setMaximumWidth(50);
        VERIFY(connect(_caFileBrowseButton, SIGNAL(clicked()), this, SLOT(on_caFileBrowseButton_clicked())));
        VERIFY(connect(_acceptSelfSignedButton, SIGNAL(toggled(bool)), this, SLOT(on_acceptSelfSignedButton_toggle(bool))));

        // Client Cert section
        _useClientCertCheckBox = new QCheckBox("Use Client Certificate ( --sslPemKeyFile )");
        _clientCertLabel = new QLabel("Client Certificate: ");
        _clientCertPathLineEdit = new QLineEdit;
        _clientCertFileBrowseButton = new QPushButton("...");
        _clientCertFileBrowseButton->setMaximumWidth(50);
        VERIFY(connect(_clientCertFileBrowseButton, SIGNAL(clicked()), this, SLOT(on_pemKeyFileBrowseButton_clicked())));

        _clientCertPassLabel = new QLabel("Passphrase: ");
        _clientCertPassLineEdit = new QLineEdit;
        _clientCertPassShowButton = new QPushButton("Show");
        VERIFY(connect(_clientCertPassShowButton, SIGNAL(clicked()), this, SLOT(togglePassphraseShowMode())));
        _useClientCertPassCheckBox = new QCheckBox("Client certificated is protected by passphrase");

        // Advanced options
        _allowInvalidHostnamesCheckBox = new QCheckBox("Allow connections to servers with non-matching hostnames");
        _allowInvalidHostnamesCheckBox->setChecked(sslSettings->allowInvalidHostnames());
        _crlFileLabel = new QLabel("CRL file: ");
        _crlFilePathLineEdit = new QLineEdit;
        _crlFileBrowseButton = new QPushButton("...");
        _crlFileBrowseButton->setMaximumWidth(50);
        VERIFY(connect(_crlFileBrowseButton, SIGNAL(clicked()), this, SLOT(on_crlFileBrowseButton_clicked())));

        // Layouts and Boxes
        // CA section
        QGridLayout* caFileLayout = new QGridLayout;
        caFileLayout->addWidget(_acceptSelfSignedButton,    0 ,0, 1, 2);
        caFileLayout->addWidget(_useRootCaFileButton,       1, 0);
        caFileLayout->addWidget(_caFilePathLineEdit,        1, 1);
        caFileLayout->addWidget(_caFileBrowseButton,        1, 2);
        QGroupBox* caFileBox = new QGroupBox(tr("CA (Certificate Authority) File"));
        caFileBox->setLayout(caFileLayout);
        caFileBox->setFont(QFont(caFileBox->font().family(), -1, -1, false));
        
        // Client Section
        QGridLayout* clientCertLayout = new QGridLayout;
        clientCertLayout->addWidget(_clientCertLabel,               0, 0);
        clientCertLayout->addWidget(_clientCertPathLineEdit,        0, 1);
        clientCertLayout->addWidget(_clientCertFileBrowseButton,    0, 2);
        clientCertLayout->addWidget(_clientCertPassLabel,           1, 0);
        clientCertLayout->addWidget(_clientCertPassLineEdit,        1, 1);
        clientCertLayout->addWidget(_clientCertPassShowButton,      1, 2);
        clientCertLayout->addWidget(_useClientCertPassCheckBox,     2, 0, 2, 2);
        QGroupBox* clientPemBox = new QGroupBox(tr("Client Certificate/Key File"));
        clientPemBox->setLayout(clientCertLayout);
        //clientPemBox->setCheckable(true);
        //clientPemBox->setChecked(false);

        // Advanced section
        QGridLayout* advancedLayout = new QGridLayout;
        advancedLayout->addWidget(_crlFileLabel,                    0, 0);
        advancedLayout->addWidget(_crlFilePathLineEdit,             0, 1);
        advancedLayout->addWidget(_crlFileBrowseButton,             0, 2);
        advancedLayout->addWidget(_allowInvalidHostnamesCheckBox,   1, 0, 1, 2);
        QGroupBox* advancedBox = new QGroupBox(tr("Advanced Options"));
        advancedBox->setLayout(advancedLayout);

        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->addWidget(_useSslCheckBox);
        mainLayout->addWidget(caFileBox);
        mainLayout->addWidget(clientPemBox);
        mainLayout->addWidget(advancedBox);
        setLayout(mainLayout);

        // Update widgets according to settings
        useSslCheckBoxStateChange(_useSslCheckBox->checkState());
        _caFilePathLineEdit->setText(QString::fromStdString(sslSettings->caFile()));
        _clientCertPathLineEdit->setText(QString::fromStdString(sslSettings->pemKeyFile()));
        _clientCertPassLineEdit->setText(QString::fromStdString(sslSettings->pemPassPhrase()));
        _allowInvalidHostnamesCheckBox->setChecked(sslSettings->allowInvalidHostnames());
        _crlFilePathLineEdit->setText(QString::fromStdString(sslSettings->crlFile()));
    }

    void SSLTab::on_acceptSelfSignedButton_toggle(bool checked)
    {
        _useRootCaFileButton->setChecked(!checked);
        setDisabledCAfileWidgets(checked);
    }

    void SSLTab::on_caFileBrowseButton_clicked()
    {
        // If user has previously selected a file, initialize file dialog
        // with that file's name; otherwise, use user's home directory.
        QString initialName = _caFilePathLineEdit->text();
        if (initialName.isEmpty())
        {
            initialName = QDir::homePath();
        }
        QString fileName =  QFileDialog::getOpenFileName(this, tr("Choose File"));
        fileName = QDir::toNativeSeparators(fileName);
        if (!fileName.isEmpty()) {
            _caFilePathLineEdit->setText(fileName);
            //buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        }

        //mongo::sslGlobalParams.sslMode.store(mongo::SSLParams::SSLMode_requireSSL);
        ////mongo::sslGlobalParams.sslPEMKeyFile = "C:\\cygwin\\etc\\ssl\\mongodb.pem";
        //mongo::sslGlobalParams.sslCAFile = "C:\\cygwin\\demarcek_pass\\mongodb.pem";
        //mongo::sslGlobalParams.sslPEMKeyFile = "C:\\cygwin\\demarcek_pass\\client.pem";
        //mongo::sslGlobalParams.sslPEMKeyPassword = "testc";

        //mongo::DBClientConnection *conn = new mongo::DBClientConnection(true, 10);
        ////mongo::Status status = conn->connect(mongo::HostAndPort("46.101.137.132", 27020));

        //try{
        //    mongo::Status status = conn->connect(mongo::HostAndPort("localhost", 27017));
        //    _clientCertPassLineEdit->setText("isOk: " + QString::number(status.isOK()));
        //}
        //catch (const std::exception& ex){
        //    _clientCertPassLineEdit->setText("exc: " + QString::fromStdString(ex.what()));
        //}

        //mongo::sslGlobalParams.sslMode.store(mongo::SSLParams::SSLMode_allowSSL);
    }

    void SSLTab::on_pemKeyFileBrowseButton_clicked()
    {
        // todo: move to function
        // If user has previously selected a file, initialize file dialog
        // with that file's name; otherwise, use user's home directory.
        QString initialName = _clientCertPathLineEdit->text();
        if (initialName.isEmpty())
        {
            initialName = QDir::homePath();
        }
        QString fileName = QFileDialog::getOpenFileName(this, tr("Choose File"));
        fileName = QDir::toNativeSeparators(fileName);
        if (!fileName.isEmpty()) {
            _clientCertPathLineEdit->setText(fileName);
            //buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        }
    }

    void SSLTab::on_crlFileBrowseButton_clicked()
    {
        // todo: move to function
        // If user has previously selected a file, initialize file dialog
        // with that file's name; otherwise, use user's home directory.
        QString initialName = _crlFilePathLineEdit->text();
        if (initialName.isEmpty())
        {
            initialName = QDir::homePath();
        }
        QString fileName = QFileDialog::getOpenFileName(this, tr("Choose File"));
        fileName = QDir::toNativeSeparators(fileName);
        if (!fileName.isEmpty()) {
            _crlFilePathLineEdit->setText(fileName);
            //buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        }
    }

    void SSLTab::useSslCheckBoxStateChange(int state)
    {
        bool isChecked = static_cast<bool>(state);
        _acceptSelfSignedButton->setDisabled(!isChecked);
        _useRootCaFileButton->setDisabled(!isChecked);
        if (state)  // if SSL enabled, disable/enable conditionally; otherwise disable all widgets.
        {
            setDisabledCAfileWidgets(_acceptSelfSignedButton->isChecked());
        }
        else
        {
            setDisabledCAfileWidgets(true);
        }
        _clientCertLabel->setDisabled(!isChecked);
        _clientCertPathLineEdit->setDisabled(!isChecked);
        _clientCertFileBrowseButton->setDisabled(!isChecked);
        _clientCertPassLabel->setDisabled(!isChecked);
        _clientCertPassLineEdit->setDisabled(!isChecked);
        _clientCertPassShowButton->setDisabled(!isChecked);
        _useClientCertCheckBox->setDisabled(!isChecked);
        _useClientCertPassCheckBox->setDisabled(!isChecked);
        _allowInvalidHostnamesCheckBox->setDisabled(!isChecked);
        _crlFileLabel->setDisabled(!isChecked);
        _crlFilePathLineEdit->setDisabled(!isChecked);
        _crlFileBrowseButton->setDisabled(!isChecked);
    }

    void SSLTab::togglePassphraseShowMode()
    {
        bool isPassword = _clientCertPassLineEdit->echoMode() == QLineEdit::Password;
        _clientCertPassLineEdit->setEchoMode(isPassword ? QLineEdit::Normal : QLineEdit::Password);
        _clientCertPassShowButton->setText(isPassword ? tr("Hide") : tr("Show"));
    }

    bool SSLTab::accept()
    {
        SslSettings *sslSettings = _settings->sslSettings();
        sslSettings->enableSSL(_useSslCheckBox->isChecked());
        sslSettings->setCaFile(QtUtils::toStdString(_caFilePathLineEdit->text()));
        sslSettings->setPemKeyFile(QtUtils::toStdString(_clientCertPathLineEdit->text()));
        sslSettings->setPemPassPhrase(QtUtils::toStdString(_clientCertPassLineEdit->text()));
        sslSettings->setAllowInvalidCertificates(_acceptSelfSignedButton->isChecked());
        sslSettings->setAllowInvalidHostnames(_allowInvalidHostnamesCheckBox->isChecked());
        sslSettings->setCrlFile(QtUtils::toStdString(_crlFilePathLineEdit->text()));
        return true;
    }

    void SSLTab::setDisabledCAfileWidgets(bool disabled)
    {
        //_caFileLabel->setDisabled(disabled);
        _caFilePathLineEdit->setDisabled(disabled);
        _caFileBrowseButton->setDisabled(disabled);
    }
}

