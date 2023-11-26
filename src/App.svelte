<script>
	import {
		onMount
	} from 'svelte';
	import Connect from './components/Connect.svelte';
	import SelectScan from './components/SelectScan.svelte';
	import Status from './components/Status.svelte';

  let scanningCounter = 0;
  let stateGettingCounter = 0;
  let stateGettingInterval = 1;

  let scanning = false;
  let connecting = false;
	let data = {
		connectStatus: {
			sent: false,
			staIP:""
		},
		selection: {
			direct_connect: false,
			selected: false,
			ssid: ""
		},
		access_points: []
	};

	function setConnectSuccess(event){
		data.connectStatus.sent = true;
		data.connectStatus.staIP = event.detail.staIP;
	}

	async function setConnecting() {

		data.connectStatus.staIP = "";
    stateGettingInterval = 10;
    stateGettingCounter = 0;

		await startStateGetting(false);
	}

	function setConnectError(){
		data.connectStatus.sent = true;
		data.connectStatus.staIP = "";
	}

	function clearSelection() {
    data.connectStatus.sent = false;
		data.selection.selected = false;
		data.selection.ssid = "";
		data.selection.direct_connect = false;
	}

	function selectAccessPoint(event) {

		data.selection.ssid = event.detail.ssid
		if(event.detail.open){
			data.selection.direct_connect = true
		}
		data.selection.selected = true;
	}

	async function refreshScanList (){ 
		data.connectStatus.sent = false;
		data.selection.selected = false;
		await startScanning()
	}

	async function erase (){ 
    const res = await fetch(`/espconnect/erase`);
    refreshScanList();
	} 

  // 添加网络状态变化监听器
  window.addEventListener('online', () => {
    console.log('Network is online');
    // 在网络恢复后重新启动扫描

  });

  window.addEventListener('offline', () => {
    console.log('Network is offline');
  });
  
  async function startScanning() 
  {
    scanningCounter = 0;
    scanning = true;

    const scanFunction = async () => 
    {
      try {
        console.log("try scanning...");
        const res = await fetch(`/espconnect/scan`);
        console.log("/espconnect/scan has finish");
        if (res.status === 200) {
          data.access_points = await res.json();
          scanning = false;
          console.log("scanning finished.");
        } else if (res.status === 202 && ++scanningCounter < 200) {
          console.log("set timeout to scanning...");
          setTimeout(scanFunction, 50);
          console.log("set timeout to scanning has finish");
        } else {
          scanning = false;
        }
      } catch (err) {
        console.log(err);
        scanning = false;
      }
    };

    await scanFunction();
  }


  async function startStateGetting(needScan)
  {
    connecting = true;
    const stateGetFunction = async (needScan) => {
      try {
        const res = await fetch(`/espconnect/STAState`);
        let stateData = await res.json();
        if (res.status === 200) {
          data.connectStatus.sent = true;
          data.connectStatus.staIP = stateData.staIP;
          connecting = false;
        } else if (res.status === 202 && ++stateGettingCounter < stateGettingInterval) {
          setTimeout(stateGetFunction, 1000, needScan);
        } else {
          if (needScan) {
            scanningCounter = 0;
            await startScanning(); // 扫描
          }
          connecting = false;
        }
      } catch (err) {
        console.log(err);
        connecting = false;
      }
    };
    await stateGetFunction(needScan);
  }

	onMount(async () => {
		try {
      stateGettingCounter = 0;
			await startStateGetting(true);
		} catch (err) {
			console.log(err);
		}
	});
</script>

<div class="container main-container">
	<div class="row">
		<div class="column text-center">
			<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 24 24" class="logo"><path d="M5 12L3 12 3 21 12 21 12 19 5 19zM12 5L19 5 19 12 21 12 21 3 12 3z"></path></svg>
		</div>
	</div>
	<div class="row mb-2">
		<div class="column">
			<div class="card">
				{#if scanning || connecting }
					<div class="container">
						<div class="row h-100">
							<div class="column text-center d-flex h-100">
								<div class="loader">
									<svg xmlns="http://www.w3.org/2000/svg" width="36" height="36" viewBox="0 0 24 24" style="fill: currentColor"><path d="M12,22c5.421,0,10-4.579,10-10h-2c0,4.337-3.663,8-8,8s-8-3.663-8-8c0-4.336,3.663-8,8-8V2C6.579,2,2,6.58,2,12 C2,17.421,6.579,22,12,22z"></path></svg>
								</div>
							</div>
						</div>
					</div>
				{:else}
					{#if data.connectStatus.sent} 
						<Status staIP={data.connectStatus.staIP} on:back={clearSelection} on:erase={erase}/>
					{:else}
						{#if data.selection.selected}
							<Connect ssid={data.selection.ssid} direct_connect={data.selection.direct_connect} staIP={data.connectStatus.staIP} on:back={clearSelection} on:success={setConnectSuccess} on:connecting={setConnecting} on:error={setConnectError} />
						{:else}
							<SelectScan access_points={data.access_points} on:refresh={refreshScanList} on:select={selectAccessPoint} />
						{/if}
					{/if}
				{/if}
			</div>
		</div>
	</div>
	<div class="row">
		<div class="column text-sm text-muted">
			<p class="text-center">
				Made with ❤️ by <a href="https://github.com/ayushsharma82" target="_blank">ayushsharma82</a>.
			</p>
			<p class="text-center">
				<a href="https://www.buymeacoffee.com/6QGVpSj" target="_blank">Buy me a coffee ☕</a>
			</p>
		</div>
	</div>
</div>

<style type="text/scss" global>
	$color-primary: #353a41;
	$color-secondary: #202327;
	$color-muted: #626770;
	$color-quinary: #869ab8;
	
	$loader-color: #fff;
	$loader-size: 8px;
	$loader-height: 4px;
	$loader-border-size: 4px;
	$loader-gap: 6px;
	$loader-animation-duration: 1s;

	@import "../node_modules/milligram/src/milligram.sass";
	@import "../node_modules/spinthatshit/src/variables.scss";
	@import "../node_modules/spinthatshit/src/animations.scss";
	@import "../node_modules/spinthatshit/src/loaders/_loader10.scss";

	.text-muted{
		color: $color-muted !important;
	}

	.text-center{
		text-align: center;
	}

	.w-100{
		width: 100%;
	}

	.h-100{
		height: 100%;
	}

	.d-flex{
		display: flex !important;
	}

	.flex-columns{
		flex-direction: column;
	}

	.flex-rows{
		flex-direction: row;
	}

	.mb-2{
		margin-bottom: 2rem;
	}

	.pr-1{
		padding-right: 1rem;
	}

	.w-auto{
		width: auto !important;
	}

	.text-sm{
		font-size: 12px;
	}

	.clickable-row{
		padding: 1rem 0rem;
		border-bottom: 0.1rem solid #f4f5f6;
		transition: background-color .5s cubic-bezier(0.215, 0.610, 0.355, 1), box-shadow .5s cubic-bezier(0.215, 0.610, 0.355, 1);
		border-radius: .5rem;
		cursor: pointer;
		word-break: break-word;

		&:hover{
			box-shadow: rgba(99, 99, 99, 0.1) 0px 2px 8px 2px;
		}
	}

	input[type=text], input[type=password]{
		padding: 2.5rem 2rem !important;
		box-shadow: rgba(204, 219, 232, 0.2) 0px 3px 6px 1px inset, rgba(255, 255, 255, 0.4) 0px 0px 6px 6px inset;
		border: none;
		font-size: 16px !important;
	}

	input:disabled{
		background-color: rgba(47, 63, 82, 0.05);
		border: none;
	}

	.logo{
		margin: 5rem 1rem;
		fill: currentColor;
	}

	.main-container{
		max-width: 42rem !important;
		margin: 3rem auto;
	}

	.card{
		display: flex;
		position: relative;
		padding: 1rem;
		border-radius: 1rem;
		width: 100%;
		box-shadow: rgba(149, 157, 165, 0.15) 0px 8px 24px;
		min-height: 290px;

		.container{
			padding: 1rem 2rem;
		}
	}

	.button{
		border-radius: .5rem;
		transition: background-color .5s linear;
		box-shadow: rgba(149, 157, 165, 0.15) 0px 8px 24px !important;
		&[disabled]{
			opacity: 0.9;
		}
	}

	.btn-light{
		background-color:rgb(113, 121, 129) !important;
		border-color: rgb(113, 121, 129) !important;
	}

	.loader{
		margin: auto;
		svg {
			animation-name: spin;
			animation-duration: 1500ms;
			animation-iteration-count: infinite;
			animation-timing-function: linear; 
		}
	}

	@keyframes spin {
    from {
        transform:rotate(0deg);
    }
    to {
        transform:rotate(360deg);
    }
	}

	.btn-loader{
		margin: auto;
		@include loader10;
	}

</style>
