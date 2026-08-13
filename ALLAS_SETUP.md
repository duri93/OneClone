# COMPLETE ALLAS SETUP GUIDE

This is the guide to set up everything from scratch. You can skip most of these steps if you already set up Allas and rclone previously.

Along the setup you will generate a few names/codes that you need to remember. Fill them in as you go:

| Item                  | Value                    |
|-----------------------|--------------------------|
| CSC PROJECT           |                          |
| BUCKET NAME           |                          |
| AWS_ACCESS_KEY_ID     |                          |
| AWS_SECRET_ACCESS_KEY |                          |
| REMOTE NAME           |                          |

**Remember!** Delete this info when you're done with the setup: it is sensitive information that can lead to people accessing your data!

---

## Install required software

1. **Install rclone** from company portal.
   This is the backend and will manage all data transfer from and to Allas (and the vast majority of cloud storages, e.g. Google Drive and Dropbox).

2. **Install WinFsp** from company portal, too.
   This software manages virtual filesystems. In short, it can make a remote folder appear as a drive (or other folder) on your PC.

3. **Install OneClone** from the official GitHub page.
   This creates a graphical interface to let you set up and run rclone commands without interacting with the command line.

## Set up Allas to accept connections

Note: operations such as creating an account and a project can take a while to update in CSC.

4. **CSC Account**.
   * Login to [my.csc.fi](https://my.csc.fi) to access your dashboard.
   * You should use Haka and log in with your Microsoft account.
   
   > See [this guide](https://docs.csc.fi/accounts/how-to-create-new-user-account/) for help setting up your account.

5. **CSC Project**.
   * From [my.csc.fi](https://my.csc.fi), go to Menu > Projects.
   * Make sure you have a project in which you want to store your data (you can create one easily).
   * In the project, check the `Services` section and ensure there is **Allas** (the storage itself) and **Roihu** (or any supercomputer we use to connect to Allas).
   * Note down the id of your project (e.g. `project_1234567`).

6. **Allas Bucket**.
   * From the project, under services, find Allas and click login.
   * Choose Haka and login.
   * Here you will see your buckets; identify the one you want to use or create a new one.

   > Note: You can save [allas.csc.fi](https://allas.csc.fi) as a bookmark on your browser to access the online view of your data.

   > Note: buckets are automatically shared with all the members of the project — take advantage of this to share data with your group.

7. **S3 connection to Allas**.
   * Login to [roihu.csc.fi](https://roihu.csc.fi).
   * In the dashboard choose "Login to node shell (Roihu-CPU)". A command line will appear.
   * Type `module load allas` (and press enter), then `allas-conf -m S3`. 
   * When asked, type your CSC password (note: nothing will appear while you type, but the password will be recorded correctly). 
   
   > If you get a UNAUTHORIZED error, the password was probably wrong.

   * After a few moments you should see something like:
   
   `rclone remote s3allas: now provides an S3 based connection to project project_1234567 in Allas`.

8. **Allas connection credentials**.
   * In the same terminal, type `less ~/.aws/credentials`.
   * A file will open, note down the two codes (AWS_ACCESS_KEY_ID and AWS_SECRET_ACCESS_KEY) 
   * Close the browser window.

   > Note: these are secret personal codes that give full access to your data, so be careful not to share them.

## Set up the Allas connection on your laptop

An **rclone remote** is the configuration of a connection that you do once and then use every day. A single remote can access all your buckets.

A **job** is a command you can run (e.g. copy all data from my PC to Allas). This too can be set up once and used every day.

9. **Rclone remote setup**. 
    * Open OneClone.
    > A blue security message may appear the first time; click "More info", then "Run anyway".
    * Go to the Settings tab, then click "Open rclone conf".
    * A command line for the configuration of rclone should appear. Follow these steps:

      1. ` n ` to create a new remote, and give it an appropriate name (e.g. `allas`). **Remember this name — you'll need it in step 10.**
      2. ` 4 ` (Amazon S3) as connection type.
      3. ` 47 ` (Other) as provider.
      4. ` 1 ` to enter your credentials.
      5. Enter the AWS_ACCESS_KEY_ID you saved in step 8. You can right-click to paste into the terminal.
      6. Enter the AWS_SECRET_ACCESS_KEY you saved in step 8.
      7. ` 1 ` to use v4 signatures.
      8. Enter `a3s.fi` as endpoint.
      9. Press enter to leave `location_constraint` blank.
      10. ` 1 ` to keep your remote private.
      11. ` n ` to leave default configuration as-is.
      12. ` y ` to save the new remote.
      13. ` q ` to quit the configuration.

10. **Set up your jobs on OneClone**. This is the last step.
   > You can configure as many jobs as you like (e.g. sync laptop to Allas, sync external drive to Allas, sync laptop to external drive, mount Allas as network storage, etc). Each job can then be toggled on/off with a single click.

   1. On the Jobs tab, click Add.
   2. Give a name to your job (for your own reference).
   3. Choose the type of job (i.e. the underlying rclone command that will be executed):
       
      > **copy** jobs will copy data from the local to the remote folder (or vice versa).
       
      > **sync** jobs will ensure the remote folder is identical to the local one (copies new/modified files, but also deletes files on the remote if necessary).
       
      > **mount** jobs will create a virtual drive or folder on your PC. You can then use the normal Windows folder explorer to access and interact with files.
    
   4. Choose whether you want the job to run when the app starts (most useful for mount jobs).
   5. Define the local folder. For copy and sync jobs it needs to be a folder; for mount jobs it can also be a drive letter (e.g. `X:\`) or a network location (e.g. `\\allas\mybucket`).
   6. Define the remote folder. This should be in the format `remote:bucket`, where `remote` is the remote name you chose in **step 9.1**, and `bucket` is the name of the bucket you want to access.
   7. For mount jobs, you can choose the folder to be read-only. This way you cannot accidentally edit or delete files.
   8. Click Save.

   > You will see the new job in the list. You can start and stop each job at any time.
    
   > You can close the window while jobs are running (a tray icon lets you reopen it).

