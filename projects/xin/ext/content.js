(function () {
  'use strict';

  // 与规则包 cosmetic 区完全一致（V1.2.0）
  var SELECTORS = [
    '.adsbygoogle',
    'ins.adsbygoogle',
    '[id^="google_ads"]',
    '[id^="div-gpt-ad"]',
    '[id^="aswift_"]',
    '[id^="ad_unit"]',
    '[id^="adsense"]',
    'iframe[src*="doubleclick.net"]',
    'iframe[src*="googlesyndication.com"]',
    'iframe[src*="amazon-adsystem.com"]',
    'iframe[src*="googletagmanager.com"]',
    'iframe[id^="google_ads_iframe"]',
    '[data-ad-slot]',
    '[data-ad-client]',
    '[data-ad-unit]',
    '[data-ad-format]',
    '[class*="advert"]',
    '[id*="advert"]',
    '.ad-banner',
    '.ad-container',
    '.ad-wrapper',
    '.ad-slot',
    '.ad-box',
    '.ad_box',
    '.ad-banner-top',
    '.banner-ad',
    '.sponsor-ad',
    '.gg',
    '#gg',
    '.post_ad',
    '.tie-ad',
    '.ad_163',
    '[data-tuiguang]',
    '#content_left .ec_tuiguang',
    '.c-container[class*="ec_"]',
    '.gg1000',
    '.popup-ad',
    '.float-ad',
    '.side-ad',
    '[class*="ad-tag"]',
    '[class*="adTag"]',
    '[class*="ad-label"]',
    '[class*="adLabel"]',
    '[class*="ad-sign"]',
    '[class*="adSign"]',
    '.gg-tag',
    '.ad-mark',
    '.ad-tip',
    // 网易广告位覆盖层（"广告"二字由伪元素画在上面）
    'a.ad_hover_href',
    '[class*="ad_hover"]',
    '[class*="hover_href"]',
    '[class*="ad-tag-"]',
    '[class*="ad_tag"]'
  ];

  var JOINED = SELECTORS.join(',');
  var STYLE_ID = 'xin-force-style';
  var mode = 'force';

  function injectStyle() {
    if (document.getElementById(STYLE_ID)) return;
    var s = document.createElement('style');
    s.id = STYLE_ID;
    s.textContent = JOINED +
      '{display:none !important;visibility:hidden !important;' +
      'height:0 !important;width:0 !important;min-height:0 !important;' +
      'margin:0 !important;padding:0 !important;border:0 !important;}';
    var t = document.head || document.documentElement;
    if (t) t.appendChild(s);
  }

  function removeStyle() {
    var s = document.getElementById(STYLE_ID);
    if (s) s.parentNode.removeChild(s);
  }

  // 猎杀"广告"二字的合规标识：文本节点 + 伪元素（innerText）+ 叶子节点兜底
  var LABELS = { '广告': 1, '廣告': 1, '广告位': 1, 'AD': 1, 'Advertisement': 1, '赞助': 1 };
  var DBG = false;
  try { DBG = location.search.indexOf('xin-debug') >= 0; } catch (e) {}

  function isLabelText(s) {
    s = (s || '').replace(/\s+/g, '');
    if (!s || s.length > 10) return false;
    return !!LABELS[s];
  }

  var AD_CLASS_RE = /(^|[\s_-])(ad|ads|gg|guanggao)([\s_-]|$)/i;

  function hideEl(el, why) {
    if (!el || el.nodeType !== 1) return 0;
    if (el.getAttribute('data-xin-hidden') === '1') return 0;
    var w = el.offsetWidth || 0;
    var h = el.offsetHeight || 0;
    var cls = '';
    try { cls = (el.className || '') + ''; } catch (e) { cls = ''; }
    // class 明显是广告相关的元素，放宽尺寸限制（如铺满广告位的覆盖层）
    var isAdish = AD_CLASS_RE.test(cls);
    var maxW = isAdish ? 1600 : 520;
    var maxH = isAdish ? 1200 : 260;
    if (w > maxW || h > maxH) return 0;
    el.setAttribute('data-xin-hidden', '1');
    el.style.setProperty('display', 'none', 'important');
    if (DBG) console.log('[xin] hide', why, el.tagName, el.className, 'size', w + 'x' + h);
    return 1;
  }

  function killLabels() {
    if (mode !== 'force' || !document.body) return 0;
    var done = 0;

    // A. 纯文本节点（快）
    var walker;
    try {
      walker = document.createTreeWalker(document.body, NodeFilter.SHOW_TEXT, null, false);
    } catch (e) { return 0; }
    var node;
    while ((node = walker.nextNode())) {
      if (isLabelText(node.nodeValue)) done += hideEl(node.parentElement, 'text');
    }

    // B. 伪元素 / 拆分文本：按 class 关键词预筛，读 innerText（含 ::before 内容）
    var cands;
    try {
      cands = document.querySelectorAll(
        '[class*="ad"],[class*="Ad"],[class*="AD"],[class*="gg"],' +
        '[class*="tag"],[class*="label"],[class*="mark"],[class*="tip"],[class*="sign"]'
      );
    } catch (e) { cands = []; }
    for (var i = 0; i < cands.length; i++) {
      var el = cands[i];
      if (el.getAttribute('data-xin-hidden') === '1') continue;
      if (el.childElementCount > 3) continue;
      var it = '';
      try { it = el.innerText || ''; } catch (e) { it = ''; }
      if (!isLabelText(it)) continue;
      done += hideEl(el, 'pseudo/class');
    }
    return done;
  }

  // C. 兜底：所有叶子节点，用 innerText 抓伪元素（较重，只在加载后跑几次）
  function killLabelsDeep() {
    if (mode !== 'force' || !document.body) return 0;
    var done = 0;
    var all = document.body.getElementsByTagName('*');
    for (var j = 0; j < all.length; j++) {
      var e2 = all[j];
      if (e2.childElementCount !== 0) continue;
      if (e2.getAttribute('data-xin-hidden') === '1') continue;
      var tc = e2.textContent || '';
      if (tc.replace(/\s+/g, '').length > 10) continue;
      var it2 = '';
      try { it2 = e2.innerText || ''; } catch (e) { continue; }
      if (!isLabelText(it2)) continue;
      done += hideEl(e2, 'leaf');
    }
    return done;
  }

  function sweep() {
    if (mode !== 'force') return 0;
    var n = 0;
    var els;
    try { els = document.querySelectorAll(JOINED); } catch (e) { return 0; }
    for (var i = 0; i < els.length; i++) {
      var el = els[i];
      if (el.getAttribute('data-xin-hidden') === '1') continue;
      el.setAttribute('data-xin-hidden', '1');
      el.style.setProperty('display', 'none', 'important');
      n++;
    }
    n += killLabels();
    return n;
  }

  function apply() {
    if (mode === 'force') {
      injectStyle();
      sweep();
    } else {
      removeStyle();
      var old = document.querySelectorAll('[data-xin-hidden="1"]');
      for (var i = 0; i < old.length; i++) {
        old[i].removeAttribute('data-xin-hidden');
        old[i].style.removeProperty('display');
      }
    }
  }

  function boot() {
    try {
      chrome.storage.local.get(['mode'], function (r) {
        if (r && r.mode) mode = r.mode;
        apply();
      });
      chrome.storage.onChanged.addListener(function (ch) {
        if (ch && ch.mode) { mode = ch.mode.newValue; apply(); }
      });
    } catch (e) { /* 非扩展环境，忽略 */ }
    apply();
  }

  boot();
  document.addEventListener('DOMContentLoaded', apply);
  window.addEventListener('load', apply);

  var timer = null;
  try {
    var mo = new MutationObserver(function () {
      if (timer) return;
      timer = setTimeout(function () { timer = null; sweep(); }, 150);
    });
    mo.observe(document.documentElement, { childList: true, subtree: true });
  } catch (e) {}

  // 加载后多轮深度清扫（伪元素只在渲染后才有 innerText）
  var BURSTS = [600, 1500, 3000, 6000, 10000];
  for (var bi = 0; bi < BURSTS.length; bi++) {
    (function (ms) {
      setTimeout(function () {
        if (mode !== 'force') return;
        sweep();
        killLabelsDeep();
      }, ms);
    })(BURSTS[bi]);
  }
})();
