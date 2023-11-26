<script>
  import { createEventDispatcher } from 'svelte';
  const dispatch = createEventDispatcher();

  export let staIP;
  let currentURL = window.location.href; // 获取当前网页的地址

  // 判断当前网页的地址是否与指定的 IP 地址相同
  let isSameIP = currentURL.includes(staIP);

  // 新增按钮点击事件处理函数
  function gotoLocation() {
    if (staIP) {
      // 如果 staIP 存在，执行跳转到该 IP 地址的动作
      window.location.href = `http://${staIP}`;
    } 
  }

  function erase() {
    // 显示确认框
    const userConfirmed = window.confirm("将清除当前WiFi连接信息,需重新连接热点配网,继续吗?");

    // 根据用户选择执行后续动作
    if (userConfirmed) {
      // 用户点击了确认按钮，执行后续动作
      console.log("用户点击了确认按钮");
      // 在这里执行你的后续动作
      dispatch('erase');
    } else {
      // 用户点击了取消按钮，不执行后续动作
      console.log("用户点击了取消按钮");
    }
  }

  function back() {
    dispatch('back');
  }
</script>

<div class="container d-flex flex-columns">
  <div class="row h-100">
    <div class="column text-center h-100" style="margin-bottom: 1rem;">
      {#if staIP}
        <div class="d-flex h-100 flex-columns" style="margin: auto; justify-content: center;">
          <p>
            <svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 24 24" style="fill:#16c79a"><path d="M19.965,8.521C19.988,8.347,20,8.173,20,8c0-2.379-2.143-4.288-4.521-3.965C14.786,2.802,13.466,2,12,2 S9.214,2.802,8.521,4.035C6.138,3.712,4,5.621,4,8c0,0.173,0.012,0.347,0.035,0.521C2.802,9.215,2,10.535,2,12 s0.802,2.785,2.035,3.479C4.012,15.653,4,15.827,4,16c0,2.379,2.138,4.283,4.521,3.965C9.214,21.198,10.534,22,12,22 s2.786-0.802,3.479-2.035C17.857,20.283,20,18.379,20,16c0-0.173-0.012-0.347-0.035-0.521C21.198,14.785,22,13.465,22,12 S21.198,9.215,19.965,8.521z M10.955,16.416l-3.667-3.714l1.424-1.404l2.257,2.286l4.327-4.294l1.408,1.42L10.955,16.416z"></path></svg>
          </p>
          <h6>
            WiFi连接成功
          </h6>
        </div>
      {:else}
        <div class="d-flex h-100 flex-columns" style="margin: auto; justify-content: center;">
          <p>
            <svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 24 24" style="fill:#ec4646"><path d="M12,2C6.486,2,2,6.486,2,12s4.486,10,10,10s10-4.486,10-10S17.514,2,12,2z M13,17h-2v-6h2V17z M13,9h-2V7h2V9z"></path></svg>
          </p>
          <h6>
            WiFi连接失败
          </h6>
        </div>
      {/if}
    </div>
  </div>

  <!-- 新增按钮 -->
  {#if staIP && !isSameIP }
    <div class="row h-100">
      <div class="column text-center h-100" style="margin-bottom: 1rem;">
        <button class="button w-100"  on:click={gotoLocation}>
          前往 {staIP}
        </button>
      </div>
    </div>
  {/if}


  {#if staIP }
    <div class="row h-100">
      <div class="column text-center h-100" style="margin-bottom: 1rem;">
          <button class="button w-100"  on:click={erase}>
            清除当前WiFi连接
          </button>
      </div>
    </div>
  {/if}

  <div class="row h-100">
    <div class="column text-center h-100" style="margin-bottom: 1rem;">
      <button class="button w-100"  on:click={back}>
        返回
      </button>
    </div>
  </div>
</div>