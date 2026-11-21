(function () {
  'use strict';

  var radios = document.querySelectorAll('input[name="mode"]');
  var lbSoft = document.getElementById('lb-soft');
  var lbForce = document.getElementById('lb-force');
  var stat = document.getElementById('stat');
  var hasExt = (typeof chrome !== 'undefined') && chrome.storage && chrome.storage.local;

  function paint(m) {
    for (var i = 0; i < radios.length; i++) {
      radios[i].checked = (radios[i].value === m);
    }
    lbSoft.classList.toggle('on', m === 'soft');
    lbForce.classList.toggle('on', m === 'force');
    stat.textContent = '当前生效：' + (m === 'force' ? '强迫模式（广告框全删）' : '柔和模式（仅拦网络层）');
  }

  if (!hasExt) {
    paint('force');
    stat.textContent = '（预览模式：不在扩展环境中）';
    return;
  }

  chrome.storage.local.get(['mode'], function (r) {
    var m = (r && r.mode) ? r.mode : 'force';
    paint(m);
  });

  for (var i = 0; i < radios.length; i++) {
    radios[i].addEventListener('change', function (e) {
      var m = e.target.value;
      chrome.storage.local.set({ mode: m }, function () {
        paint(m);
        stat.textContent += ' · 已保存，刷新页面生效';
      });
    });
  }
})();
