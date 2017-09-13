=============================================
Installing the Desktop Synchronization Client
=============================================

You can download the  latest version of the ownCloud Desktop Synchronization 
Client from the `ownCloud download page 
<https://owncloud.org/install/#desktop>`_. 
There are clients for Linux, Mac OS X, and Microsoft Windows.

Installation on Mac OS X and Windows is the same as for any software 
application: download the program and then double-click it to launch the 
installation, and then follow the installation wizard. After it is installed and 
configured the sync client will automatically keep itself updated; see 
:doc:`autoupdate` for more information.

Linux users must follow the instructions on the download page to add the 
appropriate repository for their Linux distribution, install the signing key, 
and then use their package managers to install the desktop sync client. Linux 
users will also update their sync clients via package manager, and the client 
will display a notification when an update is available. 

Linux users must also have a password manager enabled, such as GNOME Keyring or
KWallet, so that the sync client can login automatically.

You will also find links to source code archives and older versions on the 
download page.

Customizing the Windows installation
----------------------------------

If you just want to install ownCloud Desktop Synchronization Client on your local
system, you can simply launch the .msi file and configure it in the wizard
that pops up.

Features
^^^^^^^^

The MSI installer provides several features that can be installed or removed
individually, which you can also control via command-line, if you are automating
the installation::

   msiexec /passive /install ownCloud-x.y.z.msi

will install the ownCloud Desktop Synchronization Client into the default location
with the default features enabled. If you want to disable e.g. desktop shortcut
icons you can simply change the above command to::

   msiexec /passive /install ownCloud-x.y.z.msi REMOVE=DesktopShortcut

See the following table for a list of available features:

+--------------------+--------------------+----------------------------------+
| Feature            | Enabled by default | Description                      |
+====================+====================+==================================+
| Client             | Yes                | The actual client                |
+--------------------+--------------------+----------------------------------+
| DesktopShortcut    | Yes                | Adds a shortcut to the desktop   |
+--------------------+--------------------+----------------------------------+
| StartMenuShortcuts | Yes                | Adds shortcuts to the start menu |
+--------------------+--------------------+----------------------------------+
| ShellExtensions    | Yes                | Adds Explorer integration        |
+--------------------+--------------------+----------------------------------+

You can also choose to only install the client itself by using the following command::

  msiexec /passive /install ownCloud-x.y.z.msi ADDDEFAULT=Client

Windows keeps track of the installed features and `REMOVE=DesktopShortcut` will take
care of removing this feature, whereas `ADDDEFAULT=Client` won't remove unspecified
features on upgrades.
Compare `REMOVE <https://msdn.microsoft.com/en-us/library/windows/desktop/aa371194(v=vs.85).aspx>`_
and `ADDDEFAULT <https://msdn.microsoft.com/en-us/library/windows/desktop/aa367518(v=vs.85).aspx>`_
on the Windows Installer Guide.

Installation folder
^^^^^^^^^^^^^^^^^^^

You can adjust the installation folder by specifying the `INSTALLDIR`
property like this::

  msiexec /passive /install ownCloud-x.y.z.msi INSTALLDIR="C:\Program Files (x86)\Non Standard ownCloud Client Folder"

Be careful when using PowerShell instead of `cmd.exe`, it can be tricky to get
the whitespace escaping right there. Specifying the `INSTALLDIR` like this
only works on first installation, you cannot simply reinvoke the .msi with a
different path. If you still need to change it, uninstall it first and reinstall
it with the new path.

Disabling automatic updates
^^^^^^^^^^^^^^^^^^^^^^^^^^^
To disable automatic updates, you can pass the `SKIPAUTOUPDATE` property.::

    msiexec /passive /install ownCloud-x.y.z.msi SKIPAUTOUPDATE="1"


Launch after installation
^^^^^^^^^^^^^^^^^^^^^^^^^^^

To launch the client automatically after installation, you can pass the `LAUNCH` property.::

    msiexec /install ownCloud-x.y.z.msi LAUNCH="1"

This option also removes the checkbox to let users decide if they want to launch the client
for non passive/quiet mode.
`NOTE:` If the ownCloud client is running, it's restarted after upgrades anyway. 
`ǸOTE:` This option does not work without a UI, support for that is going to be added soon.

Installation Wizard
-------------------

The installation wizard takes you step-by-step through configuration options and 
account setup. First you need to enter the URL of your ownCloud server.

.. image:: images/client-1.png
   :alt: form for entering ownCloud server URL
   
Enter your ownCloud login on the next screen.

.. image:: images/client-2.png
   :alt: form for entering your ownCloud login

On the Local Folder Option screen you may sync 
all of your files on the ownCloud server, or select individual folders. The 
default local sync folder is ``ownCloud``, in your home directory. You may 
change this as well.

.. image:: images/client-3.png
   :alt: Select which remote folders to sync, and which local folder to store 
    them in.
   
When you have completed selecting your sync folders, click the Connect button 
at the bottom right. The client will attempt to connect to your ownCloud 
server, and when it is successful you'll see two buttons: one to connect to 
your ownCloud Web GUI, and one to open your local folder. It will also start 
synchronizing your files.

.. image:: images/client-4.png
   :alt: A successful server connection, showing a button to connect to your 
    Web GUI, and one to open your local ownCloud folder

Click the Finish button, and you're all done. 
