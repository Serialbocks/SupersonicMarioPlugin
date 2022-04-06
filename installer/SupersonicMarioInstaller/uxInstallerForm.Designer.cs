
namespace SupersonicMarioInstaller
{
    partial class uxInstallerForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(uxInstallerForm));
            this.uxDivider2 = new System.Windows.Forms.Label();
            this.uxCancel = new System.Windows.Forms.Button();
            this.uxTopBanner = new System.Windows.Forms.Label();
            this.uxInstallerTitle = new System.Windows.Forms.Label();
            this.uxDivider1 = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.uxBanner = new System.Windows.Forms.Label();
            this.uxTabs = new System.Windows.Forms.TabControl();
            this.uxWelcomePage = new System.Windows.Forms.TabPage();
            this.uxLicensePage = new System.Windows.Forms.TabPage();
            this.uxNext = new System.Windows.Forms.Button();
            this.uxBack = new System.Windows.Forms.Button();
            this.uxWelcomeMessage = new System.Windows.Forms.Label();
            this.uxBakkesmodPage = new System.Windows.Forms.TabPage();
            this.uxLicense = new System.Windows.Forms.TextBox();
            this.uxReviewTerms = new System.Windows.Forms.Label();
            this.uxAcceptLabel = new System.Windows.Forms.Label();
            this.uxTabs.SuspendLayout();
            this.uxWelcomePage.SuspendLayout();
            this.uxLicensePage.SuspendLayout();
            this.SuspendLayout();
            // 
            // uxDivider2
            // 
            this.uxDivider2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.uxDivider2.BackColor = System.Drawing.SystemColors.ControlLight;
            this.uxDivider2.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.uxDivider2.Location = new System.Drawing.Point(-1, 336);
            this.uxDivider2.Name = "uxDivider2";
            this.uxDivider2.Size = new System.Drawing.Size(510, 2);
            this.uxDivider2.TabIndex = 1;
            // 
            // uxCancel
            // 
            this.uxCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.uxCancel.Location = new System.Drawing.Point(421, 350);
            this.uxCancel.Name = "uxCancel";
            this.uxCancel.Size = new System.Drawing.Size(75, 23);
            this.uxCancel.TabIndex = 5;
            this.uxCancel.Text = "Cancel";
            this.uxCancel.UseVisualStyleBackColor = true;
            this.uxCancel.Click += new System.EventHandler(this.uxCancel_Click);
            // 
            // uxTopBanner
            // 
            this.uxTopBanner.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.uxTopBanner.BackColor = System.Drawing.SystemColors.ButtonHighlight;
            this.uxTopBanner.Location = new System.Drawing.Point(-4, -2);
            this.uxTopBanner.Name = "uxTopBanner";
            this.uxTopBanner.Size = new System.Drawing.Size(513, 73);
            this.uxTopBanner.TabIndex = 6;
            // 
            // uxInstallerTitle
            // 
            this.uxInstallerTitle.AutoSize = true;
            this.uxInstallerTitle.BackColor = System.Drawing.SystemColors.ButtonHighlight;
            this.uxInstallerTitle.Font = new System.Drawing.Font("Verdana", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.uxInstallerTitle.Location = new System.Drawing.Point(12, 19);
            this.uxInstallerTitle.Name = "uxInstallerTitle";
            this.uxInstallerTitle.Size = new System.Drawing.Size(236, 18);
            this.uxInstallerTitle.TabIndex = 7;
            this.uxInstallerTitle.Text = "Supersonic Mario Installer";
            // 
            // uxDivider1
            // 
            this.uxDivider1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.uxDivider1.BackColor = System.Drawing.SystemColors.ControlLight;
            this.uxDivider1.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.uxDivider1.Location = new System.Drawing.Point(-1, 71);
            this.uxDivider1.Name = "uxDivider1";
            this.uxDivider1.Size = new System.Drawing.Size(510, 2);
            this.uxDivider1.TabIndex = 8;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.BackColor = System.Drawing.SystemColors.ButtonHighlight;
            this.label1.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(12, 37);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(132, 13);
            this.label1.TabIndex = 9;
            this.label1.Text = "A mod by Serialbocks";
            // 
            // uxBanner
            // 
            this.uxBanner.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.uxBanner.BackColor = System.Drawing.Color.White;
            this.uxBanner.Image = global::SupersonicMarioInstaller.Properties.Resources.banner_small;
            this.uxBanner.Location = new System.Drawing.Point(332, 9);
            this.uxBanner.Name = "uxBanner";
            this.uxBanner.Size = new System.Drawing.Size(164, 52);
            this.uxBanner.TabIndex = 10;
            // 
            // uxTabs
            // 
            this.uxTabs.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.uxTabs.Controls.Add(this.uxWelcomePage);
            this.uxTabs.Controls.Add(this.uxLicensePage);
            this.uxTabs.Controls.Add(this.uxBakkesmodPage);
            this.uxTabs.Location = new System.Drawing.Point(-1, 74);
            this.uxTabs.Name = "uxTabs";
            this.uxTabs.SelectedIndex = 0;
            this.uxTabs.Size = new System.Drawing.Size(510, 262);
            this.uxTabs.TabIndex = 11;
            // 
            // uxWelcomePage
            // 
            this.uxWelcomePage.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.uxWelcomePage.Controls.Add(this.uxWelcomeMessage);
            this.uxWelcomePage.Location = new System.Drawing.Point(4, 22);
            this.uxWelcomePage.Name = "uxWelcomePage";
            this.uxWelcomePage.Padding = new System.Windows.Forms.Padding(3);
            this.uxWelcomePage.Size = new System.Drawing.Size(502, 236);
            this.uxWelcomePage.TabIndex = 0;
            this.uxWelcomePage.Text = "Welcome";
            // 
            // uxLicensePage
            // 
            this.uxLicensePage.Controls.Add(this.uxAcceptLabel);
            this.uxLicensePage.Controls.Add(this.uxReviewTerms);
            this.uxLicensePage.Controls.Add(this.uxLicense);
            this.uxLicensePage.Location = new System.Drawing.Point(4, 22);
            this.uxLicensePage.Name = "uxLicensePage";
            this.uxLicensePage.Padding = new System.Windows.Forms.Padding(3);
            this.uxLicensePage.Size = new System.Drawing.Size(502, 236);
            this.uxLicensePage.TabIndex = 1;
            this.uxLicensePage.Text = "License";
            this.uxLicensePage.UseVisualStyleBackColor = true;
            // 
            // uxNext
            // 
            this.uxNext.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.uxNext.Location = new System.Drawing.Point(328, 350);
            this.uxNext.Name = "uxNext";
            this.uxNext.Size = new System.Drawing.Size(75, 23);
            this.uxNext.TabIndex = 1;
            this.uxNext.Text = "Next >";
            this.uxNext.UseVisualStyleBackColor = true;
            this.uxNext.Click += new System.EventHandler(this.uxNext_Click);
            // 
            // uxBack
            // 
            this.uxBack.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.uxBack.Location = new System.Drawing.Point(247, 350);
            this.uxBack.Name = "uxBack";
            this.uxBack.Size = new System.Drawing.Size(75, 23);
            this.uxBack.TabIndex = 7;
            this.uxBack.Text = "< Back";
            this.uxBack.UseVisualStyleBackColor = true;
            this.uxBack.Visible = false;
            this.uxBack.Click += new System.EventHandler(this.uxBack_Click);
            // 
            // uxWelcomeMessage
            // 
            this.uxWelcomeMessage.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.uxWelcomeMessage.Location = new System.Drawing.Point(12, 14);
            this.uxWelcomeMessage.Name = "uxWelcomeMessage";
            this.uxWelcomeMessage.Size = new System.Drawing.Size(481, 166);
            this.uxWelcomeMessage.TabIndex = 0;
            this.uxWelcomeMessage.Text = resources.GetString("uxWelcomeMessage.Text");
            // 
            // uxBakkesmodPage
            // 
            this.uxBakkesmodPage.Location = new System.Drawing.Point(4, 22);
            this.uxBakkesmodPage.Name = "uxBakkesmodPage";
            this.uxBakkesmodPage.Padding = new System.Windows.Forms.Padding(3);
            this.uxBakkesmodPage.Size = new System.Drawing.Size(502, 236);
            this.uxBakkesmodPage.TabIndex = 2;
            this.uxBakkesmodPage.Text = "Bakkesmod";
            this.uxBakkesmodPage.UseVisualStyleBackColor = true;
            // 
            // uxLicense
            // 
            this.uxLicense.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.uxLicense.BackColor = System.Drawing.SystemColors.ButtonHighlight;
            this.uxLicense.Location = new System.Drawing.Point(12, 33);
            this.uxLicense.Multiline = true;
            this.uxLicense.Name = "uxLicense";
            this.uxLicense.ReadOnly = true;
            this.uxLicense.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.uxLicense.Size = new System.Drawing.Size(481, 140);
            this.uxLicense.TabIndex = 0;
            this.uxLicense.TabStop = false;
            this.uxLicense.Text = resources.GetString("uxLicense.Text");
            // 
            // uxReviewTerms
            // 
            this.uxReviewTerms.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.uxReviewTerms.Location = new System.Drawing.Point(12, 7);
            this.uxReviewTerms.Name = "uxReviewTerms";
            this.uxReviewTerms.Size = new System.Drawing.Size(481, 23);
            this.uxReviewTerms.TabIndex = 1;
            this.uxReviewTerms.Text = "Please review the license terms before installing Supersonic Mario.";
            // 
            // uxAcceptLabel
            // 
            this.uxAcceptLabel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.uxAcceptLabel.Location = new System.Drawing.Point(12, 187);
            this.uxAcceptLabel.Name = "uxAcceptLabel";
            this.uxAcceptLabel.Size = new System.Drawing.Size(484, 34);
            this.uxAcceptLabel.TabIndex = 2;
            this.uxAcceptLabel.Text = "If you accept the terms of the agreement, click I Agree to continue. You must acc" +
    "ept the agreement to install Supersonic Mario.";
            // 
            // uxInstallerForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.ClientSize = new System.Drawing.Size(508, 385);
            this.Controls.Add(this.uxBack);
            this.Controls.Add(this.uxNext);
            this.Controls.Add(this.uxTabs);
            this.Controls.Add(this.uxBanner);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.uxDivider1);
            this.Controls.Add(this.uxInstallerTitle);
            this.Controls.Add(this.uxTopBanner);
            this.Controls.Add(this.uxCancel);
            this.Controls.Add(this.uxDivider2);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimumSize = new System.Drawing.Size(444, 411);
            this.Name = "uxInstallerForm";
            this.Text = "Supersonic Mario Installer";
            this.uxTabs.ResumeLayout(false);
            this.uxWelcomePage.ResumeLayout(false);
            this.uxLicensePage.ResumeLayout(false);
            this.uxLicensePage.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.Label uxDivider2;
        private System.Windows.Forms.Button uxCancel;
        private System.Windows.Forms.Label uxTopBanner;
        private System.Windows.Forms.Label uxInstallerTitle;
        private System.Windows.Forms.Label uxDivider1;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label uxBanner;
        private System.Windows.Forms.TabControl uxTabs;
        private System.Windows.Forms.TabPage uxWelcomePage;
        private System.Windows.Forms.TabPage uxLicensePage;
        private System.Windows.Forms.Button uxNext;
        private System.Windows.Forms.Button uxBack;
        private System.Windows.Forms.Label uxWelcomeMessage;
        private System.Windows.Forms.TabPage uxBakkesmodPage;
        private System.Windows.Forms.TextBox uxLicense;
        private System.Windows.Forms.Label uxReviewTerms;
        private System.Windows.Forms.Label uxAcceptLabel;
    }
}

