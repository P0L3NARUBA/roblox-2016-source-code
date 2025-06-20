﻿//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version:4.0.30319.42000
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

namespace Roblox.Grid.Arbiter.Common.Arbiter
{


    [System.CodeDom.Compiler.GeneratedCodeAttribute("System.ServiceModel", "4.0.0.0")]
    [System.ServiceModel.ServiceContractAttribute(Namespace = "http://roblox.com/", ConfigurationName = "Roblox.Grid.Arbiter.Common.Arbiter.Arbiter")]
    public interface Arbiter
    {

        [System.ServiceModel.OperationContractAttribute(Action = "http://roblox.com/Arbiter/GetStatsEx", ReplyAction = "http://roblox.com/Arbiter/GetStatsExResponse")]
        string GetStatsEx(bool clearExceptions);

        [System.ServiceModel.OperationContractAttribute(AsyncPattern = true, Action = "http://roblox.com/Arbiter/GetStatsEx", ReplyAction = "http://roblox.com/Arbiter/GetStatsExResponse")]
        System.IAsyncResult BeginGetStatsEx(bool clearExceptions, System.AsyncCallback callback, object asyncState);

        string EndGetStatsEx(System.IAsyncResult result);

        [System.ServiceModel.OperationContractAttribute(Action = "http://roblox.com/Arbiter/GetStats", ReplyAction = "http://roblox.com/Arbiter/GetStatsResponse")]
        string GetStats();

        [System.ServiceModel.OperationContractAttribute(AsyncPattern = true, Action = "http://roblox.com/Arbiter/GetStats", ReplyAction = "http://roblox.com/Arbiter/GetStatsResponse")]
        System.IAsyncResult BeginGetStats(System.AsyncCallback callback, object asyncState);

        string EndGetStats(System.IAsyncResult result);

        [System.ServiceModel.OperationContractAttribute(Action = "http://roblox.com/Arbiter/SetMultiProcess", ReplyAction = "http://roblox.com/Arbiter/SetMultiProcessResponse")]
        void SetMultiProcess(bool value);

        [System.ServiceModel.OperationContractAttribute(AsyncPattern = true, Action = "http://roblox.com/Arbiter/SetMultiProcess", ReplyAction = "http://roblox.com/Arbiter/SetMultiProcessResponse")]
        System.IAsyncResult BeginSetMultiProcess(bool value, System.AsyncCallback callback, object asyncState);

        void EndSetMultiProcess(System.IAsyncResult result);

        [System.ServiceModel.OperationContractAttribute(Action = "http://roblox.com/Arbiter/SetRecycleProcess", ReplyAction = "http://roblox.com/Arbiter/SetRecycleProcessResponse")]
        void SetRecycleProcess(bool value);

        [System.ServiceModel.OperationContractAttribute(AsyncPattern = true, Action = "http://roblox.com/Arbiter/SetRecycleProcess", ReplyAction = "http://roblox.com/Arbiter/SetRecycleProcessResponse")]
        System.IAsyncResult BeginSetRecycleProcess(bool value, System.AsyncCallback callback, object asyncState);

        void EndSetRecycleProcess(System.IAsyncResult result);

        [System.ServiceModel.OperationContractAttribute(Action = "http://roblox.com/Arbiter/SetThreadConfigScript", ReplyAction = "http://roblox.com/Arbiter/SetThreadConfigScriptResponse")]
        void SetThreadConfigScript(string script, bool broadcast);

        [System.ServiceModel.OperationContractAttribute(AsyncPattern = true, Action = "http://roblox.com/Arbiter/SetThreadConfigScript", ReplyAction = "http://roblox.com/Arbiter/SetThreadConfigScriptResponse")]
        System.IAsyncResult BeginSetThreadConfigScript(string script, bool broadcast, System.AsyncCallback callback, object asyncState);

        void EndSetThreadConfigScript(System.IAsyncResult result);

        [System.ServiceModel.OperationContractAttribute(Action = "http://roblox.com/Arbiter/SetRecycleQueueSize", ReplyAction = "http://roblox.com/Arbiter/SetRecycleQueueSizeResponse")]
        void SetRecycleQueueSize(int value);

        [System.ServiceModel.OperationContractAttribute(AsyncPattern = true, Action = "http://roblox.com/Arbiter/SetRecycleQueueSize", ReplyAction = "http://roblox.com/Arbiter/SetRecycleQueueSizeResponse")]
        System.IAsyncResult BeginSetRecycleQueueSize(int value, System.AsyncCallback callback, object asyncState);

        void EndSetRecycleQueueSize(System.IAsyncResult result);
    }

    [System.CodeDom.Compiler.GeneratedCodeAttribute("System.ServiceModel", "4.0.0.0")]
    public interface ArbiterChannel : Roblox.Grid.Arbiter.Common.Arbiter.Arbiter, System.ServiceModel.IClientChannel
    {
    }

    [System.Diagnostics.DebuggerStepThroughAttribute()]
    [System.CodeDom.Compiler.GeneratedCodeAttribute("System.ServiceModel", "4.0.0.0")]
    public partial class GetStatsExCompletedEventArgs : System.ComponentModel.AsyncCompletedEventArgs
    {

        private object[] results;

        public GetStatsExCompletedEventArgs(object[] results, System.Exception exception, bool cancelled, object userState) :
                base(exception, cancelled, userState)
        {
            this.results = results;
        }

        public string Result
        {
            get
            {
                base.RaiseExceptionIfNecessary();
                return ((string)(this.results[0]));
            }
        }
    }

    [System.Diagnostics.DebuggerStepThroughAttribute()]
    [System.CodeDom.Compiler.GeneratedCodeAttribute("System.ServiceModel", "4.0.0.0")]
    public partial class GetStatsCompletedEventArgs : System.ComponentModel.AsyncCompletedEventArgs
    {

        private object[] results;

        public GetStatsCompletedEventArgs(object[] results, System.Exception exception, bool cancelled, object userState) :
                base(exception, cancelled, userState)
        {
            this.results = results;
        }

        public string Result
        {
            get
            {
                base.RaiseExceptionIfNecessary();
                return ((string)(this.results[0]));
            }
        }
    }

    [System.Diagnostics.DebuggerStepThroughAttribute()]
    [System.CodeDom.Compiler.GeneratedCodeAttribute("System.ServiceModel", "4.0.0.0")]
    public partial class ArbiterClient : System.ServiceModel.ClientBase<Roblox.Grid.Arbiter.Common.Arbiter.Arbiter>, Roblox.Grid.Arbiter.Common.Arbiter.Arbiter
    {

        private BeginOperationDelegate onBeginGetStatsExDelegate;

        private EndOperationDelegate onEndGetStatsExDelegate;

        private System.Threading.SendOrPostCallback onGetStatsExCompletedDelegate;

        private BeginOperationDelegate onBeginGetStatsDelegate;

        private EndOperationDelegate onEndGetStatsDelegate;

        private System.Threading.SendOrPostCallback onGetStatsCompletedDelegate;

        private BeginOperationDelegate onBeginSetMultiProcessDelegate;

        private EndOperationDelegate onEndSetMultiProcessDelegate;

        private System.Threading.SendOrPostCallback onSetMultiProcessCompletedDelegate;

        private BeginOperationDelegate onBeginSetRecycleProcessDelegate;

        private EndOperationDelegate onEndSetRecycleProcessDelegate;

        private System.Threading.SendOrPostCallback onSetRecycleProcessCompletedDelegate;

        private BeginOperationDelegate onBeginSetThreadConfigScriptDelegate;

        private EndOperationDelegate onEndSetThreadConfigScriptDelegate;

        private System.Threading.SendOrPostCallback onSetThreadConfigScriptCompletedDelegate;

        private BeginOperationDelegate onBeginSetRecycleQueueSizeDelegate;

        private EndOperationDelegate onEndSetRecycleQueueSizeDelegate;

        private System.Threading.SendOrPostCallback onSetRecycleQueueSizeCompletedDelegate;

        public ArbiterClient()
        {
        }

        public ArbiterClient(string endpointConfigurationName) :
                base(endpointConfigurationName)
        {
        }

        public ArbiterClient(string endpointConfigurationName, string remoteAddress) :
                base(endpointConfigurationName, remoteAddress)
        {
        }

        public ArbiterClient(string endpointConfigurationName, System.ServiceModel.EndpointAddress remoteAddress) :
                base(endpointConfigurationName, remoteAddress)
        {
        }

        public ArbiterClient(System.ServiceModel.Channels.Binding binding, System.ServiceModel.EndpointAddress remoteAddress) :
                base(binding, remoteAddress)
        {
        }

        public event System.EventHandler<GetStatsExCompletedEventArgs> GetStatsExCompleted;

        public event System.EventHandler<GetStatsCompletedEventArgs> GetStatsCompleted;

        public event System.EventHandler<System.ComponentModel.AsyncCompletedEventArgs> SetMultiProcessCompleted;

        public event System.EventHandler<System.ComponentModel.AsyncCompletedEventArgs> SetRecycleProcessCompleted;

        public event System.EventHandler<System.ComponentModel.AsyncCompletedEventArgs> SetThreadConfigScriptCompleted;

        public event System.EventHandler<System.ComponentModel.AsyncCompletedEventArgs> SetRecycleQueueSizeCompleted;

        public string GetStatsEx(bool clearExceptions)
        {
            return base.Channel.GetStatsEx(clearExceptions);
        }

        [System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)]
        public System.IAsyncResult BeginGetStatsEx(bool clearExceptions, System.AsyncCallback callback, object asyncState)
        {
            return base.Channel.BeginGetStatsEx(clearExceptions, callback, asyncState);
        }

        [System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)]
        public string EndGetStatsEx(System.IAsyncResult result)
        {
            return base.Channel.EndGetStatsEx(result);
        }

        private System.IAsyncResult OnBeginGetStatsEx(object[] inValues, System.AsyncCallback callback, object asyncState)
        {
            bool clearExceptions = ((bool)(inValues[0]));
            return this.BeginGetStatsEx(clearExceptions, callback, asyncState);
        }

        private object[] OnEndGetStatsEx(System.IAsyncResult result)
        {
            string retVal = this.EndGetStatsEx(result);
            return new object[] {
                    retVal};
        }

        private void OnGetStatsExCompleted(object state)
        {
            if ((this.GetStatsExCompleted != null))
            {
                InvokeAsyncCompletedEventArgs e = ((InvokeAsyncCompletedEventArgs)(state));
                this.GetStatsExCompleted(this, new GetStatsExCompletedEventArgs(e.Results, e.Error, e.Cancelled, e.UserState));
            }
        }

        public void GetStatsExAsync(bool clearExceptions)
        {
            this.GetStatsExAsync(clearExceptions, null);
        }

        public void GetStatsExAsync(bool clearExceptions, object userState)
        {
            if ((this.onBeginGetStatsExDelegate == null))
            {
                this.onBeginGetStatsExDelegate = new BeginOperationDelegate(this.OnBeginGetStatsEx);
            }
            if ((this.onEndGetStatsExDelegate == null))
            {
                this.onEndGetStatsExDelegate = new EndOperationDelegate(this.OnEndGetStatsEx);
            }
            if ((this.onGetStatsExCompletedDelegate == null))
            {
                this.onGetStatsExCompletedDelegate = new System.Threading.SendOrPostCallback(this.OnGetStatsExCompleted);
            }
            base.InvokeAsync(this.onBeginGetStatsExDelegate, new object[] {
                        clearExceptions}, this.onEndGetStatsExDelegate, this.onGetStatsExCompletedDelegate, userState);
        }

        public string GetStats()
        {
            return base.Channel.GetStats();
        }

        [System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)]
        public System.IAsyncResult BeginGetStats(System.AsyncCallback callback, object asyncState)
        {
            return base.Channel.BeginGetStats(callback, asyncState);
        }

        [System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)]
        public string EndGetStats(System.IAsyncResult result)
        {
            return base.Channel.EndGetStats(result);
        }

        private System.IAsyncResult OnBeginGetStats(object[] inValues, System.AsyncCallback callback, object asyncState)
        {
            return this.BeginGetStats(callback, asyncState);
        }

        private object[] OnEndGetStats(System.IAsyncResult result)
        {
            string retVal = this.EndGetStats(result);
            return new object[] {
                    retVal};
        }

        private void OnGetStatsCompleted(object state)
        {
            if ((this.GetStatsCompleted != null))
            {
                InvokeAsyncCompletedEventArgs e = ((InvokeAsyncCompletedEventArgs)(state));
                this.GetStatsCompleted(this, new GetStatsCompletedEventArgs(e.Results, e.Error, e.Cancelled, e.UserState));
            }
        }

        public void GetStatsAsync()
        {
            this.GetStatsAsync(null);
        }

        public void GetStatsAsync(object userState)
        {
            if ((this.onBeginGetStatsDelegate == null))
            {
                this.onBeginGetStatsDelegate = new BeginOperationDelegate(this.OnBeginGetStats);
            }
            if ((this.onEndGetStatsDelegate == null))
            {
                this.onEndGetStatsDelegate = new EndOperationDelegate(this.OnEndGetStats);
            }
            if ((this.onGetStatsCompletedDelegate == null))
            {
                this.onGetStatsCompletedDelegate = new System.Threading.SendOrPostCallback(this.OnGetStatsCompleted);
            }
            base.InvokeAsync(this.onBeginGetStatsDelegate, null, this.onEndGetStatsDelegate, this.onGetStatsCompletedDelegate, userState);
        }

        public void SetMultiProcess(bool value)
        {
            base.Channel.SetMultiProcess(value);
        }

        [System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)]
        public System.IAsyncResult BeginSetMultiProcess(bool value, System.AsyncCallback callback, object asyncState)
        {
            return base.Channel.BeginSetMultiProcess(value, callback, asyncState);
        }

        [System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)]
        public void EndSetMultiProcess(System.IAsyncResult result)
        {
            base.Channel.EndSetMultiProcess(result);
        }

        private System.IAsyncResult OnBeginSetMultiProcess(object[] inValues, System.AsyncCallback callback, object asyncState)
        {
            bool value = ((bool)(inValues[0]));
            return this.BeginSetMultiProcess(value, callback, asyncState);
        }

        private object[] OnEndSetMultiProcess(System.IAsyncResult result)
        {
            this.EndSetMultiProcess(result);
            return null;
        }

        private void OnSetMultiProcessCompleted(object state)
        {
            if ((this.SetMultiProcessCompleted != null))
            {
                InvokeAsyncCompletedEventArgs e = ((InvokeAsyncCompletedEventArgs)(state));
                this.SetMultiProcessCompleted(this, new System.ComponentModel.AsyncCompletedEventArgs(e.Error, e.Cancelled, e.UserState));
            }
        }

        public void SetMultiProcessAsync(bool value)
        {
            this.SetMultiProcessAsync(value, null);
        }

        public void SetMultiProcessAsync(bool value, object userState)
        {
            if ((this.onBeginSetMultiProcessDelegate == null))
            {
                this.onBeginSetMultiProcessDelegate = new BeginOperationDelegate(this.OnBeginSetMultiProcess);
            }
            if ((this.onEndSetMultiProcessDelegate == null))
            {
                this.onEndSetMultiProcessDelegate = new EndOperationDelegate(this.OnEndSetMultiProcess);
            }
            if ((this.onSetMultiProcessCompletedDelegate == null))
            {
                this.onSetMultiProcessCompletedDelegate = new System.Threading.SendOrPostCallback(this.OnSetMultiProcessCompleted);
            }
            base.InvokeAsync(this.onBeginSetMultiProcessDelegate, new object[] {
                        value}, this.onEndSetMultiProcessDelegate, this.onSetMultiProcessCompletedDelegate, userState);
        }

        public void SetRecycleProcess(bool value)
        {
            base.Channel.SetRecycleProcess(value);
        }

        [System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)]
        public System.IAsyncResult BeginSetRecycleProcess(bool value, System.AsyncCallback callback, object asyncState)
        {
            return base.Channel.BeginSetRecycleProcess(value, callback, asyncState);
        }

        [System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)]
        public void EndSetRecycleProcess(System.IAsyncResult result)
        {
            base.Channel.EndSetRecycleProcess(result);
        }

        private System.IAsyncResult OnBeginSetRecycleProcess(object[] inValues, System.AsyncCallback callback, object asyncState)
        {
            bool value = ((bool)(inValues[0]));
            return this.BeginSetRecycleProcess(value, callback, asyncState);
        }

        private object[] OnEndSetRecycleProcess(System.IAsyncResult result)
        {
            this.EndSetRecycleProcess(result);
            return null;
        }

        private void OnSetRecycleProcessCompleted(object state)
        {
            if ((this.SetRecycleProcessCompleted != null))
            {
                InvokeAsyncCompletedEventArgs e = ((InvokeAsyncCompletedEventArgs)(state));
                this.SetRecycleProcessCompleted(this, new System.ComponentModel.AsyncCompletedEventArgs(e.Error, e.Cancelled, e.UserState));
            }
        }

        public void SetRecycleProcessAsync(bool value)
        {
            this.SetRecycleProcessAsync(value, null);
        }

        public void SetRecycleProcessAsync(bool value, object userState)
        {
            if ((this.onBeginSetRecycleProcessDelegate == null))
            {
                this.onBeginSetRecycleProcessDelegate = new BeginOperationDelegate(this.OnBeginSetRecycleProcess);
            }
            if ((this.onEndSetRecycleProcessDelegate == null))
            {
                this.onEndSetRecycleProcessDelegate = new EndOperationDelegate(this.OnEndSetRecycleProcess);
            }
            if ((this.onSetRecycleProcessCompletedDelegate == null))
            {
                this.onSetRecycleProcessCompletedDelegate = new System.Threading.SendOrPostCallback(this.OnSetRecycleProcessCompleted);
            }
            base.InvokeAsync(this.onBeginSetRecycleProcessDelegate, new object[] {
                        value}, this.onEndSetRecycleProcessDelegate, this.onSetRecycleProcessCompletedDelegate, userState);
        }

        public void SetThreadConfigScript(string script, bool broadcast)
        {
            base.Channel.SetThreadConfigScript(script, broadcast);
        }

        [System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)]
        public System.IAsyncResult BeginSetThreadConfigScript(string script, bool broadcast, System.AsyncCallback callback, object asyncState)
        {
            return base.Channel.BeginSetThreadConfigScript(script, broadcast, callback, asyncState);
        }

        [System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)]
        public void EndSetThreadConfigScript(System.IAsyncResult result)
        {
            base.Channel.EndSetThreadConfigScript(result);
        }

        private System.IAsyncResult OnBeginSetThreadConfigScript(object[] inValues, System.AsyncCallback callback, object asyncState)
        {
            string script = ((string)(inValues[0]));
            bool broadcast = ((bool)(inValues[1]));
            return this.BeginSetThreadConfigScript(script, broadcast, callback, asyncState);
        }

        private object[] OnEndSetThreadConfigScript(System.IAsyncResult result)
        {
            this.EndSetThreadConfigScript(result);
            return null;
        }

        private void OnSetThreadConfigScriptCompleted(object state)
        {
            if ((this.SetThreadConfigScriptCompleted != null))
            {
                InvokeAsyncCompletedEventArgs e = ((InvokeAsyncCompletedEventArgs)(state));
                this.SetThreadConfigScriptCompleted(this, new System.ComponentModel.AsyncCompletedEventArgs(e.Error, e.Cancelled, e.UserState));
            }
        }

        public void SetThreadConfigScriptAsync(string script, bool broadcast)
        {
            this.SetThreadConfigScriptAsync(script, broadcast, null);
        }

        public void SetThreadConfigScriptAsync(string script, bool broadcast, object userState)
        {
            if ((this.onBeginSetThreadConfigScriptDelegate == null))
            {
                this.onBeginSetThreadConfigScriptDelegate = new BeginOperationDelegate(this.OnBeginSetThreadConfigScript);
            }
            if ((this.onEndSetThreadConfigScriptDelegate == null))
            {
                this.onEndSetThreadConfigScriptDelegate = new EndOperationDelegate(this.OnEndSetThreadConfigScript);
            }
            if ((this.onSetThreadConfigScriptCompletedDelegate == null))
            {
                this.onSetThreadConfigScriptCompletedDelegate = new System.Threading.SendOrPostCallback(this.OnSetThreadConfigScriptCompleted);
            }
            base.InvokeAsync(this.onBeginSetThreadConfigScriptDelegate, new object[] {
                        script,
                        broadcast}, this.onEndSetThreadConfigScriptDelegate, this.onSetThreadConfigScriptCompletedDelegate, userState);
        }

        public void SetRecycleQueueSize(int value)
        {
            base.Channel.SetRecycleQueueSize(value);
        }

        [System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)]
        public System.IAsyncResult BeginSetRecycleQueueSize(int value, System.AsyncCallback callback, object asyncState)
        {
            return base.Channel.BeginSetRecycleQueueSize(value, callback, asyncState);
        }

        [System.ComponentModel.EditorBrowsableAttribute(System.ComponentModel.EditorBrowsableState.Advanced)]
        public void EndSetRecycleQueueSize(System.IAsyncResult result)
        {
            base.Channel.EndSetRecycleQueueSize(result);
        }

        private System.IAsyncResult OnBeginSetRecycleQueueSize(object[] inValues, System.AsyncCallback callback, object asyncState)
        {
            int value = ((int)(inValues[0]));
            return this.BeginSetRecycleQueueSize(value, callback, asyncState);
        }

        private object[] OnEndSetRecycleQueueSize(System.IAsyncResult result)
        {
            this.EndSetRecycleQueueSize(result);
            return null;
        }

        private void OnSetRecycleQueueSizeCompleted(object state)
        {
            if ((this.SetRecycleQueueSizeCompleted != null))
            {
                InvokeAsyncCompletedEventArgs e = ((InvokeAsyncCompletedEventArgs)(state));
                this.SetRecycleQueueSizeCompleted(this, new System.ComponentModel.AsyncCompletedEventArgs(e.Error, e.Cancelled, e.UserState));
            }
        }

        public void SetRecycleQueueSizeAsync(int value)
        {
            this.SetRecycleQueueSizeAsync(value, null);
        }

        public void SetRecycleQueueSizeAsync(int value, object userState)
        {
            if ((this.onBeginSetRecycleQueueSizeDelegate == null))
            {
                this.onBeginSetRecycleQueueSizeDelegate = new BeginOperationDelegate(this.OnBeginSetRecycleQueueSize);
            }
            if ((this.onEndSetRecycleQueueSizeDelegate == null))
            {
                this.onEndSetRecycleQueueSizeDelegate = new EndOperationDelegate(this.OnEndSetRecycleQueueSize);
            }
            if ((this.onSetRecycleQueueSizeCompletedDelegate == null))
            {
                this.onSetRecycleQueueSizeCompletedDelegate = new System.Threading.SendOrPostCallback(this.OnSetRecycleQueueSizeCompleted);
            }
            base.InvokeAsync(this.onBeginSetRecycleQueueSizeDelegate, new object[] {
                        value}, this.onEndSetRecycleQueueSizeDelegate, this.onSetRecycleQueueSizeCompletedDelegate, userState);
        }
    }
}
