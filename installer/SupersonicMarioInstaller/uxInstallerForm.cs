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
using System.Security.Cryptography;
using System.Threading;

using Microsoft.WindowsAPICodePack.Dialogs;

namespace SupersonicMarioInstaller
{
    public partial class uxInstallerForm : Form
    {
        private const string VERSION = "V0.1.0";

        private const string NEXT_BUTTON_TEXT = "Next >";
        private const string AGREE_BUTTON_TEXT = "I Agree";
        private const string INSTALL_BUTTON_TEXT = "Install";
        private const string FINISH_BUTTON_TEXT = "Finish";

        private const string SM64_MD5 = "20b854b239203baf6c961b850a4a51a2";
        private const string BAKKESMOD_SETUP_URL = "https://github.com/bakkesmodorg/BakkesModInjectorCpp/releases/latest/download/BakkesModSetup.zip";
        private const string MSYS_SETUP_URL = "https://github.com/msys2/msys2-installer/releases/download/2022-03-19/msys2-x86_64-20220319.exe";
        private const string MSYS_RELATIVE_EXE = @"usr\bin\mintty.exe";
        private const string MSYS_DEFAULT_PATH = @"C:\msys64";
        private const string MSYS_BASE_ARGS = "-w hide /bin/env MSYSTEM=MINGW64 /bin/bash -l -c \"";
        private const string LIBSM64_REPO_URL = "https://github.com/Serialbocks/libsm64-supersonic-mario.git";
        private const string LIBSM64_REPO_NAME = "libsm64-supersonic-mario";
        private const string DEFAULT_PLUGIN_CONFIG = "volume,50\r\nrom,";

        private Step _currentStep = Step.Welcome;
        private string _bakkesmodZip = "";
        private string _msysInstaller = "";
        private string _msysBasePath = MSYS_DEFAULT_PATH;

        enum Step
        {
            Welcome,
            License,
            ROM,
            Bakkesmod,
            MSYS2,
            Preinstall,
            Install,
            Postinstall
        };

        public uxInstallerForm()
        {
            InitializeComponent();

            uxVersion.Text = VERSION;

            uxTabs.Appearance = TabAppearance.FlatButtons;
            uxTabs.ItemSize = new Size(0, 1);
            uxTabs.SizeMode = TabSizeMode.Fixed;

            uxBackgroundWorker.DoWork += InstallBackground;
            uxBackgroundWorker.ProgressChanged += InstallProgressChanged;
            uxBackgroundWorker.WorkerReportsProgress = true;
        }

        private bool IsBakkesmodInstalled()
        {
            var appdata = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
            var bakkesmodPath = Path.Combine(appdata, "bakkesmod\\bakkesmod");
            return Directory.Exists(bakkesmodPath);
        }

        private bool IsMSYS2Installed()
        {
            return File.Exists(Path.Combine(_msysBasePath, MSYS_RELATIVE_EXE));
        }

        private void ExecuteMSYS2Command(string command, int startProgress)
        {
            Process msysProcess = new Process();
            msysProcess.StartInfo.FileName = Path.Combine(_msysBasePath, MSYS_RELATIVE_EXE);
            msysProcess.StartInfo.Arguments = MSYS_BASE_ARGS + command + " |& tee msys2.log\"";
            msysProcess.StartInfo.UseShellExecute = false;
            msysProcess.StartInfo.RedirectStandardOutput = true;
            msysProcess.Start();
            Thread.Sleep(100);

            int currentProgress = startProgress;
            while(msysProcess.StartTime == null)
            {
                Thread.Sleep(10);
            }

            while(msysProcess.StandardOutput.ReadLine() != null)
            {
                if(currentProgress - startProgress < 30)
                {
                    currentProgress++;
                    uxBackgroundWorker.ReportProgress(currentProgress);
                }
                Thread.Sleep(1500);
            }

        }

        private void CloseProcess(string processName)
        {
            var process = new Process();
            process.StartInfo.FileName = "CMD.exe";
            process.StartInfo.Arguments = "/C taskkill /F /IM " + processName;
            process.StartInfo.UseShellExecute = false;
            process.StartInfo.RedirectStandardOutput = true;
            process.StartInfo.CreateNoWindow = true;
            process.Start();
        }

        private void InstallBackground(object sender, System.ComponentModel.DoWorkEventArgs e)
        {
            uxInstallStatus.Invoke((MethodInvoker)delegate
            {
                uxInstallStatus.Text = "Closing Rocket League and Bakkesmod...";
            });
            CloseProcess("RocketLeague.exe");
            CloseProcess("BakkesMod.exe");
            Thread.Sleep(1000);

            uxBackgroundWorker.ReportProgress(10);
            uxInstallStatus.Invoke((MethodInvoker)delegate 
            {
                uxInstallStatus.Text = "Installing tools to build libsm64 (this step may take some time)...";
            });
            
            ExecuteMSYS2Command("pacman -S --needed --noconfirm git make python3 mingw-w64-x86_64-gcc", 0);
            uxBackgroundWorker.ReportProgress(40);

            uxInstallStatus.Invoke((MethodInvoker)delegate
            {
                uxInstallStatus.Text = "Downloading libsm64 files...";
            });

            var repoDir = Path.Combine(_msysBasePath, "home", Environment.UserName, LIBSM64_REPO_NAME);
            if (Directory.Exists(repoDir))
            {
                ExecuteMSYS2Command($"rm -rf {LIBSM64_REPO_NAME}", 40);
            }

            ExecuteMSYS2Command($"git clone {LIBSM64_REPO_URL}", 40);
            uxBackgroundWorker.ReportProgress(60);

            uxInstallStatus.Invoke((MethodInvoker)delegate
            {
                uxInstallStatus.Text = "Building libsm64...";
            });

            ExecuteMSYS2Command($"cd ./{LIBSM64_REPO_NAME} && make", 60);
            uxBackgroundWorker.ReportProgress(85);

            uxInstallStatus.Invoke((MethodInvoker)delegate
            {
                uxInstallStatus.Text = "Installing game files..";
            });

            var appdata = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
            var bakkesmodPath = Path.Combine(appdata, "bakkesmod/bakkesmod");
            var dataPath = Path.Combine(bakkesmodPath, "data");
            var assetsPath = Path.Combine(dataPath, "assets");
            var libsPath = Path.Combine(bakkesmodPath, "libs");
            var pluginsPath = Path.Combine(bakkesmodPath, "plugins");
            if(!Directory.Exists(dataPath))
            {
                Directory.CreateDirectory(dataPath);
            }
            if (!Directory.Exists(assetsPath))
            {
                Directory.CreateDirectory(assetsPath);
            }
            if (!Directory.Exists(libsPath))
            {
                Directory.CreateDirectory(libsPath);
            }
            if (!Directory.Exists(pluginsPath))
            {
                Directory.CreateDirectory(pluginsPath);
            }

            // Install built sm64.dll
            var sm64DllSource = Path.Combine(repoDir, "dist/sm64.dll");
            var sm64DllDest = Path.Combine(libsPath, "sm64.dll");
            File.Copy(sm64DllSource, sm64DllDest, true);
            ExecuteMSYS2Command($"rm -rf {LIBSM64_REPO_NAME}", 90);

            // Install resources
            File.WriteAllBytes(Path.Combine(pluginsPath, "SupersonicMario.dll"), Properties.Resources.SupersonicMario);

            File.WriteAllBytes(Path.Combine(libsPath, "sm64.lib"), Properties.Resources.sm64);
            File.WriteAllBytes(Path.Combine(libsPath, "libsoxr.dll"), Properties.Resources.libsoxr_dll);
            File.WriteAllBytes(Path.Combine(libsPath, "libsoxr.lib"), Properties.Resources.libsoxr_lib);
            File.WriteAllBytes(Path.Combine(libsPath, "libsoxr.LICENSE"), Properties.Resources.libsoxr_LICENSE);
            File.WriteAllBytes(Path.Combine(libsPath, "assimp-vc142-mt.dll"), Properties.Resources.assimp_vc142_mt_dll);
            File.WriteAllBytes(Path.Combine(libsPath, "assimp-vc142-mt.lib"), Properties.Resources.assimp_vc142_mt_lib);
            File.WriteAllBytes(Path.Combine(libsPath, "assimp.LICENSE"), Properties.Resources.assimp_LICENSE);
            File.WriteAllBytes(Path.Combine(libsPath, "libcrypto.lib"), Properties.Resources.libcrypto);
            File.WriteAllBytes(Path.Combine(libsPath, "libcrypto-3-x64.dll"), Properties.Resources.libcrypto_3_x64);
            File.WriteAllBytes(Path.Combine(libsPath, "libssl.lib"), Properties.Resources.libssl);
            File.WriteAllBytes(Path.Combine(libsPath, "libssl-3-x64.dll"), Properties.Resources.libssl_3_x64);
            File.WriteAllBytes(Path.Combine(libsPath, "openssl.LICENSE"), Properties.Resources.openssl_LICENSE);

            File.WriteAllBytes(Path.Combine(assetsPath, "aifc_decode.exe"), Properties.Resources.aifc_decode);
            File.WriteAllBytes(Path.Combine(assetsPath, "assets.json"), Properties.Resources.assets);
            File.WriteAllBytes(Path.Combine(assetsPath, "extract_assets.exe"), Properties.Resources.extract_assets);
            File.WriteAllBytes(Path.Combine(assetsPath, "rom-hash.sha1"), Properties.Resources.rom_hash);
            File.WriteAllBytes(Path.Combine(assetsPath, "sm64tools.LICENSE"), Properties.Resources.sm64tools_LICENSE);
            File.WriteAllBytes(Path.Combine(assetsPath, "transparent.png"), Properties.Resources.transparent);

            File.WriteAllBytes(Path.Combine(assetsPath, "Rocketball.fbx"), Properties.Resources.Rocketball);
            File.WriteAllBytes(Path.Combine(assetsPath, "Rocketball.LICENSE"), Properties.Resources.Rocketball_LICENSE);
            File.WriteAllBytes(Path.Combine(assetsPath, "Octane.fbx"), Properties.Resources.Octane);
            File.WriteAllBytes(Path.Combine(assetsPath, "Octane.LICENSE"), Properties.Resources.Octane_LICENSE);
            File.WriteAllBytes(Path.Combine(assetsPath, "Dominus.fbx"), Properties.Resources.Dominus);
            File.WriteAllBytes(Path.Combine(assetsPath, "Dominus.LICENSE"), Properties.Resources.Dominus_LICENSE);
            File.WriteAllBytes(Path.Combine(assetsPath, "Fennec.fbx"), Properties.Resources.Fennec);
            File.WriteAllBytes(Path.Combine(assetsPath, "Fennec.LICENSE"), Properties.Resources.Fennec_LICENSE);

            // Copy game config
            var cfgContents = DEFAULT_PLUGIN_CONFIG + uxRomPath.Text + "\r\n";
            File.WriteAllText(Path.Combine(dataPath, "supersonicmario.cfg"), cfgContents);

            uxInstallStatus.Invoke((MethodInvoker)delegate
            {
                Next();
            });
        }

        private void InstallProgressChanged(object sender, System.ComponentModel.ProgressChangedEventArgs e)
        {
            uxInstallProgress.Value = e.ProgressPercentage;
        }

        private void Install()
        {
            uxBackgroundWorker.RunWorkerAsync();
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
                case Step.ROM:
                    uxBack.Visible = true;
                    uxNext.Visible = true;
                    uxOpenFileDialog.Filter = "z64 files (*.z64)|*.z64";
                    uxNext.Enabled = uxRomPath.Text.Length > 0;
                    uxNext.Text = NEXT_BUTTON_TEXT;
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
                case Step.MSYS2:
                    uxBack.Visible = true;
                    uxNext.Visible = true;
                    uxBack.Enabled = true;
                    uxNext.Enabled = false;
                    uxMsys2Label.Visible = true;
                    uxInstallMsys2.Visible = true;
                    uxSelectMsysInstallDir.Visible = true;
                    uxMSYSProgress.Visible = false;
                    uxMsysStatus.Text = "";
                    uxNext.Text = NEXT_BUTTON_TEXT;
                    break;
                case Step.Preinstall:
                    uxBack.Visible = true;
                    uxNext.Visible = true;
                    uxBack.Enabled = true;
                    uxNext.Enabled = true;
                    uxNext.Text = INSTALL_BUTTON_TEXT;
                    break;
                case Step.Install:
                    uxBack.Visible = false;
                    uxNext.Visible = false;
                    break;
                case Step.Postinstall:
                    uxNext.Visible = false;
                    uxBack.Visible = false;
                    uxCancel.Text = FINISH_BUTTON_TEXT;
                    break;
                default:
                    break;
            }
        }

        private void uxCancel_Click(object sender, EventArgs e)
        {

            Application.Exit();
        }

        private void Next()
        {
            _currentStep++;

            switch (_currentStep)
            {
                case Step.ROM:
                    {
                        var appdata = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
                        var configPath = Path.Combine(appdata, "bakkesmod/bakkesmod/data/supersonicmario.cfg");
                        if(File.Exists(configPath))
                        {
                            Next();
                            return;
                        }
                    }
                    break;
                case Step.Bakkesmod:
                    if (IsBakkesmodInstalled())
                    {
                        Next();
                        return;
                    }
                    break;
                case Step.MSYS2:
                    if(IsMSYS2Installed())
                    {
                        Next();
                        return;
                    }
                    break;
                case Step.Install:
                    if (MessageBox.Show("To continue, this setup will close any running instance of Rocket League and Bakkesmod. Is that okay?",
                        "Setup", MessageBoxButtons.YesNo) == DialogResult.Yes)
                    {
                        Install();
                    }
                    else
                    {
                        Back();
                        return;
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
                        Back();
                        return;
                    }
                    break;
                case Step.MSYS2:
                    if (IsMSYS2Installed())
                    {
                        Back();
                        return;
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
                client.DownloadFileAsync(new Uri(BAKKESMOD_SETUP_URL), _bakkesmodZip);
            }
        }

        private void BakkesDownloadProgressChanged(object sender, DownloadProgressChangedEventArgs e)
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

            while(!bakkesSetupProcess.HasExited)
            {
                Application.DoEvents();
            }
            bakkesSetupProcess.WaitForExit();

            Directory.Delete(setupDir, true);

            Next();
        }

        private void uxInstallMsys2_Click(object sender, EventArgs e)
        {
            uxMsys2Label.Visible = false;
            uxInstallMsys2.Visible = false;
            uxSelectMsysInstallDir.Visible = false;
            uxMSYSProgress.Visible = true;
            uxBack.Enabled = false;
            uxNext.Enabled = false;

            uxMsysStatus.Text = "Downloading MSYS2 Installer";

            var downloadDir = Path.Combine(Path.GetTempPath(), "supersonic-mario-installer");
            if (Directory.Exists(downloadDir))
            {
                Directory.Delete(downloadDir, true);
            }
            Directory.CreateDirectory(downloadDir);

            _msysInstaller = Path.Combine(downloadDir, "msys2-x86_64-20220319.exe");
            using (var client = new WebClient())
            {
                client.DownloadProgressChanged += MsysDownloadProgressChanged;
                client.DownloadFileCompleted += new AsyncCompletedEventHandler(MsysDownloadComplete);
                client.DownloadFileAsync(new Uri(MSYS_SETUP_URL), _msysInstaller);
            }
        }

        private void MsysDownloadProgressChanged(object sender, DownloadProgressChangedEventArgs e)
        {
            uxMSYSProgress.Value = e.ProgressPercentage;
        }

        private void MsysDownloadComplete(object sender, AsyncCompletedEventArgs e)
        {
            if (e.Cancelled || e.Error != null)
            {
                MessageBox.Show("MSYS2 download failed. Check your connection and try again.", "Setup");
                UpdateUX();
                return;
            }

            uxMsysStatus.Text = "Installing - Please complete the MSYS2 installation";

            Process msysSetupProcess = new Process();
            msysSetupProcess.StartInfo.FileName = _msysInstaller;
            msysSetupProcess.Start();
            while(!msysSetupProcess.HasExited)
            {
                Application.DoEvents();
            }
            msysSetupProcess.WaitForExit();

            Directory.Delete(Path.GetDirectoryName(_msysInstaller), true);

            if (IsMSYS2Installed())
            {
                Next();
            }
            else
            {
                UpdateUX();
            }
        }

        private void uxSelectMsysInstallDir_Click(object sender, EventArgs e)
        {
            CommonOpenFileDialog dialog = new CommonOpenFileDialog();
            dialog.IsFolderPicker = true;
            if(dialog.ShowDialog() == CommonFileDialogResult.Ok)
            {
                _msysBasePath = dialog.FileName;
                if(IsMSYS2Installed())
                {
                    Next();
                }
                else
                {
                    _msysBasePath = MSYS_DEFAULT_PATH;
                    MessageBox.Show("Provided path was not a valid installation of MSYS2", "Error");
                }
            }
        }

        private void uxRomBrowse_Click(object sender, EventArgs e)
        {
            if(uxOpenFileDialog.ShowDialog() == DialogResult.OK)
            {
                string md5 = CalculateMD5(uxOpenFileDialog.FileName);
                if(md5 == SM64_MD5)
                {
                    uxRomPath.Text = uxOpenFileDialog.FileName;
                    uxNext.Enabled = true;
                }
                else
                {
                    MessageBox.Show("The ROM provided is not valid. Please make sure you're using the US version.", "Error");
                }
            }
        }

        private string CalculateMD5(string filename)
        {
            using (var md5 = MD5.Create())
            {
                using (var stream = File.OpenRead(filename))
                {
                    var hash = md5.ComputeHash(stream);
                    return BitConverter.ToString(hash).Replace("-", "").ToLowerInvariant();
                }
            }
        }

        private DialogResult WarnCloseInstaller()
        {
            if(_currentStep == Step.Postinstall)
            {
                return DialogResult.Yes;
            }
            return MessageBox.Show("Are you sure you want to exit the installation?",
                "Exit Installer",
                MessageBoxButtons.YesNo);
        }

        private void uxInstallerForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            if(WarnCloseInstaller() == DialogResult.No)
            {
                e.Cancel = true;
            }
        }
    }
}
