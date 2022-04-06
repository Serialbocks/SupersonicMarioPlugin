using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace SupersonicMarioInstaller
{
    public partial class uxInstallerForm : Form
    {
        private const string NEXT_BUTTON_TEXT = "Next >";
        private const string AGREE_BUTTON_TEXT = "I Agree";
        private const string BACK_BUTTON_TEXT = "< Back";

        private Step _currentStep = Step.Welcome;

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

        private void UpdateButtons()
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
                    uxNext.Text = AGREE_BUTTON_TEXT;
                    break;
                case Step.Bakkesmod:
                    uxBack.Visible = true;
                    uxNext.Visible = false;
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

        private void uxNext_Click(object sender, EventArgs e)
        {
            _currentStep++;
            uxTabs.SelectedIndex = (int)_currentStep;
            UpdateButtons();
        }

        private void uxBack_Click(object sender, EventArgs e)
        {
            _currentStep--;
            uxTabs.SelectedIndex = (int)_currentStep;
            UpdateButtons();
        }
    }
}
