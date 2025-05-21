function toggleDataField() {
    const t = document.getElementById('data');
    t.style.display=(t.style.display === 'none'||!t.style.display) ? 'block' : 'none';
}
  
async function sendRequest(method) {
    const url=document.getElementById('url').value.trim();
    const data=document.getElementById('data').value;
    const out=document.getElementById('response');
  
    if(!url){out.textContent='Error: Please enter a URL.';return;}
  
    try {
      const opts = {method};
      if (method==='POST' && data) {
        opts.headers = {'Content-Type' : 'application/json'};
        opts.body = data;
      }
      const res = await fetch(url,opts);
      const txt = await res.text();
      out.textContent = `Status: ${res.status} ${res.statusText}\n\n${txt}`;
    }
    catch(e) {
      out.textContent=`Error: ${e.message}`;
    }
}
