Public Class Main
    Inherits System.Windows.Forms.Form

#Region " Windows Form Designer generated code "

    Public Sub New()
        MyBase.New()

        'This call is required by the Windows Form Designer.
        InitializeComponent()

        'Add any initialization after the InitializeComponent() call

    End Sub

    'Form overrides dispose to clean up the component list.
    Protected Overloads Overrides Sub Dispose(ByVal disposing As Boolean)
        If disposing Then
            If Not (components Is Nothing) Then
                components.Dispose()
            End If
        End If
        MyBase.Dispose(disposing)
    End Sub

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents Button1 As System.Windows.Forms.Button
    Friend WithEvents Button2 As System.Windows.Forms.Button
    Friend WithEvents Button3 As System.Windows.Forms.Button
    Friend WithEvents Button4 As System.Windows.Forms.Button
    Friend WithEvents Button5 As System.Windows.Forms.Button
    <System.Diagnostics.DebuggerStepThrough()> Private Sub InitializeComponent()
        Me.Label1 = New System.Windows.Forms.Label
        Me.Button1 = New System.Windows.Forms.Button
        Me.Button2 = New System.Windows.Forms.Button
        Me.Button3 = New System.Windows.Forms.Button
        Me.Button4 = New System.Windows.Forms.Button
        Me.Button5 = New System.Windows.Forms.Button
        Me.SuspendLayout()
        '
        'Label1
        '
        Me.Label1.Anchor = CType(((System.Windows.Forms.AnchorStyles.Top Or System.Windows.Forms.AnchorStyles.Left) _
                    Or System.Windows.Forms.AnchorStyles.Right), System.Windows.Forms.AnchorStyles)
        Me.Label1.Font = New System.Drawing.Font("Microsoft Sans Serif", 18.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label1.Location = New System.Drawing.Point(24, 16)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(528, 40)
        Me.Label1.TabIndex = 0
        Me.Label1.Text = "Aardvark I2C/SPI Activity Board"
        Me.Label1.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'Button1
        '
        Me.Button1.Location = New System.Drawing.Point(16, 72)
        Me.Button1.Name = "Button1"
        Me.Button1.Size = New System.Drawing.Size(152, 40)
        Me.Button1.TabIndex = 1
        Me.Button1.Text = "aadetect"
        '
        'Button2
        '
        Me.Button2.Location = New System.Drawing.Point(200, 72)
        Me.Button2.Name = "Button2"
        Me.Button2.Size = New System.Drawing.Size(152, 40)
        Me.Button2.TabIndex = 2
        Me.Button2.Text = "aalights"
        '
        'Button3
        '
        Me.Button3.Location = New System.Drawing.Point(392, 72)
        Me.Button3.Name = "Button3"
        Me.Button3.Size = New System.Drawing.Size(152, 40)
        Me.Button3.TabIndex = 3
        Me.Button3.Text = "aagpio"
        '
        'Button4
        '
        Me.Button4.Location = New System.Drawing.Point(104, 128)
        Me.Button4.Name = "Button4"
        Me.Button4.Size = New System.Drawing.Size(152, 40)
        Me.Button4.TabIndex = 4
        Me.Button4.Text = "aai2c_eeprom"
        '
        'Button5
        '
        Me.Button5.Location = New System.Drawing.Point(296, 128)
        Me.Button5.Name = "Button5"
        Me.Button5.Size = New System.Drawing.Size(152, 40)
        Me.Button5.TabIndex = 5
        Me.Button5.Text = "aaspi_eeprom"
        '
        'Main
        '
        Me.AutoScaleBaseSize = New System.Drawing.Size(5, 13)
        Me.ClientSize = New System.Drawing.Size(568, 181)
        Me.Controls.Add(Me.Button5)
        Me.Controls.Add(Me.Button4)
        Me.Controls.Add(Me.Button3)
        Me.Controls.Add(Me.Button2)
        Me.Controls.Add(Me.Button1)
        Me.Controls.Add(Me.Label1)
        Me.Name = "Main"
        Me.Text = "Aardvark I2C/SPI Activity Board"
        Me.ResumeLayout(False)

    End Sub

#End Region

    Private Sub Button1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button1.Click
        aadetect.aadetect_run()
    End Sub

    Private Sub Button2_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button2.Click
        aalights.aalights_run()
    End Sub

    Private Sub Button3_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button3.Click
        aagpio.aagpio_run()
    End Sub

    Private Sub Button4_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button4.Click
        aai2c_eeprom.aai2c_eeprom_run()
    End Sub

    Private Sub Button5_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button5.Click
        aaspi_eeprom.aaspi_eeprom_run()
    End Sub
End Class
