using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace RDM_msg_queue
{
    public partial class Form1 : Form
    {
        MessageQueue.msgqueue msgQ;

        public Form1()
        {
            InitializeComponent();
            startMsgQueue();
        }

        void startMsgQueue()
        {
            msgQ = new MessageQueue.msgqueue();
            msgQ.updateEvent += new MessageQueue.msgqueue.updateEventHandler(msgQ_updateEvent);
        }

        void msgQ_updateEvent(object sender, MessageQueue.msgqueue.MyEventArgs eventArgs)
        {
            addLog(eventArgs.msg);
        }

        delegate void SetTextCallback(string text);
        public void addLog(string text)
        {
            // InvokeRequired required compares the thread ID of the
            // calling thread to the thread ID of the creating thread.
            // If these threads are different, it returns true.
            if (this.txtLog.InvokeRequired)
            {
                SetTextCallback d = new SetTextCallback(addLog);
                this.Invoke(d, new object[] { text });
            }
            else
            {
                if (txtLog.Text.Length > 10000)
                    txtLog.Text = "";
                txtLog.Text += text + "\r\n";
                txtLog.SelectionLength = 0;
                txtLog.SelectionStart = txtLog.Text.Length - 1;
                txtLog.ScrollToCaret();
            }
        }
        private void Form1_Closed(object sender, EventArgs e)
        {
            if (msgQ != null)
            {
                msgQ.Dispose();
                msgQ = null;
            }
        }
    }
}