const { chromium } = require('playwright');

(async () => {
  const url = process.env.WEBGPU_TEST_URL || 'http://127.0.0.1:8000/index.html';
  const browser = await chromium.launch({
    headless: true,
    args: [
      '--enable-unsafe-webgpu',
      '--disable-dawn-features=disallow_unsafe_apis',
    ],
  });
  const page = await browser.newPage();
  await page.goto(url, { waitUntil: 'networkidle' });
  const statusText = await page.textContent('#status');
  await browser.close();

  if (!statusText || !statusText.includes('WebGPU OK')) {
    console.error(`WebGPU smoke test failed: ${statusText}`);
    process.exit(1);
  }
  console.log('WebGPU smoke test passed');
})();
