<script>
  import { onMount, createEventDispatcher } from 'svelte';
  const dispatch = createEventDispatcher();

  export let ssid;
  export let direct_connect;
  export let staIP;
  let loading = false;
  let password = "";

  async function connect(){
    if (staIP) {
      // 显示确认框
      const userConfirmed = window.confirm("当前已连接Wifi,切换WiFi将会断开连接,继续吗?");

      // 根据用户选择执行后续动作
      if (userConfirmed) {
        // 用户点击了确认按钮，执行后续动作
        console.log("用户点击了确认按钮");
      } else {
        // 用户点击了取消按钮，不执行后续动作
        console.log("用户点击了取消按钮");
        return;
      }
    }

    loading = true;
    let formData = new FormData();
    formData.append('ssid', ssid);
    formData.append('password', password);
    const res = await fetch(`/espconnect/connect`, { method: 'POST', body: formData });
		let stateData = await res.json();
		if (res.status === 200) {
      dispatch('success', { staIP: stateData.staIP });
		} if (res.status === 202) {
      dispatch('connecting');
		} else {
      dispatch('error');
    }
    loading = false;
		return res;
  }

  function back(){
    dispatch('back')
  }

  onMount(async () => {
    if(direct_connect){
      loading = true;
      await connect();
      loading = false;
    }
  })
</script>

<form class="container d-flex flex-columns" on:submit|preventDefault={connect}>
  <div class="row h-100">
    <div class="column">
      <div class="row">
        <div class="column text-center">
          <p class="mb-2 text-muted">
            Connect to WiFi
          </p>
        </div>
      </div>
      <div class="row">
        <div class="column column-100">
          <input type="text" placeholder="SSID" id="ssid" value={ssid} disabled required>
        </div>
      </div>
      <div class="row">
        <div class="column column-100">
          <input type="password" placeholder="WiFi Password" id="password" bind:value={password} disabled={loading} required minlength="8">
        </div>
      </div>
    </div>
  </div>
  <div class="row flex-rows">
    <div class="column w-auto pr-1">
      <button type="button" class="button btn-light text-center" on:click={back} disabled={loading}>
        <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" style="fill: #fff; vertical-align: middle"><path d="M21 11L6.414 11 11.707 5.707 10.293 4.293 2.586 12 10.293 19.707 11.707 18.293 6.414 13 21 13z"></path></svg>
      </button>
    </div>
    <div class="column">
      <button type="submit" class="button w-100 text-center" disabled={loading}>
        {#if loading}
          <div class="btn-loader"></div>
        {:else}
          Connect
        {/if}
      </button>
    </div>
  </div>
</form>