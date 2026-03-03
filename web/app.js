(async () => {
  const status = document.getElementById('status');
  if (!('gpu' in navigator)) {
    status.textContent = 'WebGPU not available';
    status.classList.add('fail');
    return;
  }

  try {
    const adapter = await navigator.gpu.requestAdapter();
    if (!adapter) {
      status.textContent = 'WebGPU adapter not found';
      status.classList.add('fail');
      return;
    }

    const device = await adapter.requestDevice();
    if (!device) {
      status.textContent = 'WebGPU device not created';
      status.classList.add('fail');
      return;
    }

    status.textContent = 'WebGPU OK';
    status.classList.add('ok');
  } catch (err) {
    status.textContent = `WebGPU error: ${err}`;
    status.classList.add('fail');
  }
})();
