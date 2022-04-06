
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
            this.SuspendLayout();
            // 
            // uxDivider2
            // 
            this.uxDivider2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.uxDivider2.BackColor = System.Drawing.SystemColors.ControlLight;
            this.uxDivider2.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.uxDivider2.Location = new System.Drawing.Point(-1, 403);
            this.uxDivider2.Name = "uxDivider2";
            this.uxDivider2.Size = new System.Drawing.Size(599, 2);
            this.uxDivider2.TabIndex = 1;
            // 
            // uxCancel
            // 
            this.uxCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.uxCancel.Location = new System.Drawing.Point(510, 417);
            this.uxCancel.Name = "uxCancel";
            this.uxCancel.Size = new System.Drawing.Size(75, 23);
            this.uxCancel.TabIndex = 5;
            this.uxCancel.Text = "Cancel";
            this.uxCancel.UseVisualStyleBackColor = true;
            // 
            // uxTopBanner
            // 
            this.uxTopBanner.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.uxTopBanner.BackColor = System.Drawing.SystemColors.ButtonHighlight;
            this.uxTopBanner.Location = new System.Drawing.Point(-4, -2);
            this.uxTopBanner.Name = "uxTopBanner";
            this.uxTopBanner.Size = new System.Drawing.Size(602, 73);
            this.uxTopBanner.TabIndex = 6;
            // 
            // uxInstallerTitle
            // 
            this.uxInstallerTitle.AutoSize = true;
            this.uxInstallerTitle.BackColor = System.Drawing.SystemColors.ButtonHighlight;
            this.uxInstallerTitle.Font = new System.Drawing.Font("Verdana", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.uxInstallerTitle.Location = new System.Drawing.Point(12, 19);
            this.uxInstallerTitle.Name = "uxInstallerTitle";
            this.uxInstallerTitle.Size = new System.Drawing.Size(243, 18);
            this.uxInstallerTitle.TabIndex = 7;
            this.uxInstallerTitle.Text = "Installing Supersonic Mario";
            // 
            // uxDivider1
            // 
            this.uxDivider1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.uxDivider1.BackColor = System.Drawing.SystemColors.ControlLight;
            this.uxDivider1.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.uxDivider1.Location = new System.Drawing.Point(-1, 71);
            this.uxDivider1.Name = "uxDivider1";
            this.uxDivider1.Size = new System.Drawing.Size(599, 2);
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
            // uxInstallerForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.ButtonFace;
            this.ClientSize = new System.Drawing.Size(597, 452);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.uxDivider1);
            this.Controls.Add(this.uxInstallerTitle);
            this.Controls.Add(this.uxTopBanner);
            this.Controls.Add(this.uxCancel);
            this.Controls.Add(this.uxDivider2);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MinimumSize = new System.Drawing.Size(325, 325);
            this.Name = "uxInstallerForm";
            this.Text = "Supersonic Mario Installer";
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
    }
}

