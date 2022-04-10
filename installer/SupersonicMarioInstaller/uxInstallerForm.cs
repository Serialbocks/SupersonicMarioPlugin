using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using System.IO.Compression;
using System.Diagnostics;
using System.Net;

namespace SupersonicMarioInstaller
{
    public partial class uxInstallerForm : Form
    {
        private const string NEXT_BUTTON_TEXT = "Next >";
        private const string AGREE_BUTTON_TEXT = "I Agree";
        private const string BACK_BUTTON_TEXT = "< Back";

        private const string BAKKESMOD_SETUP_URL = "https://github.com/bakkesmodorg/BakkesModInjectorCpp/releases/latest/download/BakkesModSetup.zip";

        private Step _currentStep = Step.Welcome;
        private string _bakkesmodZip = "";

        enum Step
        {
            Welcome,
            License,
            Bakkesmod,
            MYSYS2,
            Install
        };

        public uxInstallerForm()
        {
            InitializeComponent();
            this.uxTabs.Appearance = TabAppearance.FlatButtons;
            this.uxTabs.ItemSize = new Size(0, 1);
            this.uxTabs.SizeMode = TabSizeMode.Fixed;
        }

        private bool IsBakkesmodInstalled()
        {
            var appdata = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
            var bakkesmodPath = Path.Combine(appdata, "bakkesmod\\bakkesmod");
            return Directory.Exists(bakkesmodPath);
        }

        private void UpdateUX()
        {
            switch(_currentStep)
            {
                case Step.Welcome:
                    uxBack.Visible = false;
                    uxNext.Visible = true;
                    uxNext.Text = NEXT_BUTTON_TEXT;
                    break;
                case Step.License:
                    uxBack.Visible = true;
                    uxNext.Visible = true;
                    uxNext.Enabled = true;
                    uxNext.Text = AGREE_BUTTON_TEXT;
                    break;
                case Step.Bakkesmod:
                    uxBack.Visible = true;
                    uxNext.Visible = true;
                    uxNext.Enabled = false;
                    uxBack.Enabled = true;
                    uxBakkesStatus.Text = "";
                    uxBakkesLabel.Visible = true;
                    uxInstallBakkesmod.Visible = true;
                    uxBakkesProgress.Visible = false;
                    uxNext.Text = NEXT_BUTTON_TEXT;
                    break;
                case Step.MYSYS2:
                    uxBack.Visible = true;
                    uxNext.Visible = true;
                    uxBack.Enabled = true;
                    uxNext.Enabled = false;
                    uxNext.Text = NEXT_BUTTON_TEXT;
                    break;
                default:
                    break;
            }
        }

        private void uxCancel_Click(object sender, EventArgs e)
        {
            var dialogResult = MessageBox.Show("Are you sure you want to exit the installation?",
                "Exit Installer",
                MessageBoxButtons.YesNo);
            if(dialogResult == DialogResult.Yes)
            {
                Application.Exit();
            }
        }

        private void Next()
        {
            _currentStep++;

            switch (_currentStep)
            {
                case Step.Bakkesmod:
                    if (IsBakkesmodInstalled())
                    {
                        _currentStep++;
                    }
                    break;
                default:
                    break;
            }

            uxTabs.SelectedIndex = (int)_currentStep;
            UpdateUX();
        }

        private void uxNext_Click(object sender, EventArgs e)
        {
            Next();
        }

        private void Back()
        {
            _currentStep--;

            switch (_currentStep)
            {
                case Step.Bakkesmod:
                    if (IsBakkesmodInstalled())
                    {
                        _currentStep--;
                    }
                    break;
                default:
                    break;
            }

            uxTabs.SelectedIndex = (int)_currentStep;
            UpdateUX();
        }

        private void uxBack_Click(object sender, EventArgs e)
        {
            Back();
        }

        private void uxInstallBakkesmod_Click(object sender, EventArgs e)
        {
            var downloadDir = Path.Combine(Path.GetTempPath(), "supersonic-mario-installer");
            if(Directory.Exists(downloadDir))
            {
                Directory.Delete(downloadDir, true);
            }
            Directory.CreateDirectory(downloadDir);

            uxBakkesLabel.Visible = false;
            uxInstallBakkesmod.Visible = false;
            uxBakkesProgress.Visible = true;
            uxBack.Enabled = false;

            uxBakkesStatus.Text = "Downloading Bakkesmod";

            _bakkesmodZip = Path.Combine(downloadDir, "BakkesmodSetup.zip");
            using (var client = new WebClient())
            {
                client.DownloadProgressChanged += BakkesDownloadProgressChanged;
                client.DownloadFileCompleted += new AsyncCompletedEventHandler(BakkesDownloadComplete);
                client.DownloadFileAsync(new System.Uri(BAKKESMOD_SETUP_URL), _bakkesmodZip);
            }
        }

        void BakkesDownloadProgressChanged(object sender, DownloadProgressChangedEventArgs e)
        {
            uxBakkesProgress.Value = e.ProgressPercentage;
        }

        private void BakkesDownloadComplete(object sender, AsyncCompletedEventArgs e)
        {
            if (e.Cancelled || e.Error != null)
            {
                MessageBox.Show("Bakkesmod download failed. Check your connection and try again.", "Setup");
                UpdateUX();
                return;
            }

            uxBakkesStatus.Text = "Extracting Setup";
            var setupDir = Path.GetDirectoryName(_bakkesmodZip);
            ZipFile.ExtractToDirectory(_bakkesmodZip, setupDir);

            uxBakkesStatus.Text = "Installing - Please complete the Bakkesmod installation";

            var bakkesSetupFilename = Path.Combine(setupDir, "BakkesModSetup.exe");
            Process bakkesSetupProcess = new Process();
            bakkesSetupProcess.StartInfo.FileName = bakkesSetupFilename;
            bakkesSetupProcess.Start();
            bakkesSetupProcess.WaitForExit();

            Directory.Delete(setupDir, true);

            MessageBox.Show("Please run Bakkesmod if it isn't already, then click OK", "Setup");

            if(IsBakkesmodInstalled())
            {
                Next();
            }
            else
            {
                MessageBox.Show("Bakkesmod setup failed or was cancelled. Please try again", "Setup");
                UpdateUX();
            }
        }
    }
}
