function GetMyDomain()
{
	var a = document.createElement("a");
	a.href = window.location.href;

	return a.host;	
}
function deleteAllCookies(strDot) {

    var cookies = document.cookie.split(";");

        for (var i = 0; i < cookies.length; i++) {
			var cookie = cookies[i];
			var eqPos = cookie.indexOf("=");
			var name = eqPos > -1 ? cookie.substr(0, eqPos) : cookie;
			document.cookie = name + "=; path=/; domain="+strDot+GetMyDomain()+"; expires=Thu, 01 Jan 1970 00:00:00 GMT";
        }
}


$(function()
{
	/*if (window.location.hostname == "www.3s3s.org")
	{
		window.location.href = window.location.protocol + "//" + "3s3s.org" + window.location.pathname
	}*/
	if  (GetMyDomain() == '3s3s.ru')
	{
		//alert('Domain 3s3s.org has been blocked in Russia ((( You can use 3s3s.ru for a while.');
		var head = '<center><h1>Этот анонимайзер скоро будет заблокирован в России</h1></center>'
		var message = 
		    '<div><table style="width:100%; table-layout: fixed; cellspacing=\'0\' cellpadding=\'0\' align=\'center\'">'+
		      '<tr><td class="middle_left">'+
		    	  '</td>'+
		    	  '<td class="middle_middle">'+
		    	  	'<h3>Госдума РФ одобрила в 3 чтении пакет законов, направленных на борьбу с анонимностью в сети.</h3><br>'+
		    		'Для обхода блокировок на территории РФ, самый простой способ теперь - <a href="https://www.torproject.org/download/download">использовать браузер TOR</a><br><br>' +
					'Для более продвинутых пользователей - можно продолжать пользоваться этим анонимайзером, но вам нужно зарегистрировать собственное доменное имя (вместо 3s3s.ru)<br>' +
					'<br>Следуйте инструкциям: <br>' +
					'1. Купите (или получие бесплатно) домен.<br>' +
					'2. Настройте A записи своего домена на IP адрес этого анонимайзера (на сегодня адрес 104.131.65.219 но как вы понимаете, все может в любой момент поменяться). Настройки должны выглядеть примерно так:<br>' +
					'А запись 1: "@.ваш.домен -> 104.131.65.219"<br>'+
					'А запись 2: "*.ваш.домен -> 104.131.65.219"<br><br>'+
					
					'Наконец, самые продвинутые могут <a href="https://habrahabr.ru/post/324290/">сделать анонимайзер на собственном сервере за 10 минут.</a><br><br><br>'+
					
					'<h3>Пока анонимайзер окончательно не заблокирован, можете свободно <a href="https://3s3s.github.io/links/">продолжать им пользоваться</a></h3>'
		    	  '</td>'+
			  '<td class="middle_right"></td>'
		      '</tr>'+
		    '</table></div>';
			
		document.body.innerHTML = 
			"<div class='main_page'>" + 
				"<div class='main_sheet'>" + 
					"<div>" + head + "</div>" + 
					"<div class='main_content content_middle'>" + message + 
				"</div>"
			"</div>";
		return;
	}	
	
	deleteAllCookies("");

	$( "#accordion" ).accordion({ header: "h2", heightStyle: "content" });
	
	$( "#tabs-top" ).tabs();
	
	$("#blocked_url").bind('keypress', function(e) {
	  if (e.which != 13) {
		   return;
	  }
	  UnblockURL($("#blocked_url").val());
	});
	
	$( "#go_unblock_url" )
		.click(function( event )
		{
			event.preventDefault();
			UnblockURL($("#blocked_url").val());			
		});
		
	$( "#make_short_url" )
		.click(function( event )
		{
			event.preventDefault();
			
			function FillRows(data)
			{
				var rows = "";
				for (var i=0; i<data.short.length; i++)
				{
					rows += "<tr><td>http://" + data.short[i] + "."+GetMyDomain()+"</td></tr>";
				}
				return rows;
			}
			
			$.post("/make_short_url.ssp", { action: "make", long: $("#long_url").val(), alias: $("#custom_alias").val() }, 
				function(data, status)
				{
					if (data.result != true)
					{
						if (data.reason == "wait24")
						{
							$("#short_links_table").remove();
							$("#short_links").append(
								"<div id='short_links_table'><span style='color:red'>"+
									"Sorry. Only one domain name in 24 hours available. Remain "+(86400 - (data.time-data.time_last)) + " seconds" +
								"</span>"+
								"<br><span>To get a short link without domain, please clean the alias field.<span>"+
								"</div>");
						}
						return;
					}
					
					$("#short_links_table").remove();
					$("#short_links").append("<table id='short_links_table'>"+FillRows(data)+"</table>");
				}, "json")
				.fail(function(xhr, textStatus, error) 
				{
					$("#short_links_table").remove();
					$("#short_links").append(
						"<div id='short_links_table'><span style='color:red'>"+
							"Operation failed!"+
						"</div>");
				});
		});
});

function UnblockURL(strBlocked)
{
	var RKN_List = ["legalrc.biz", "www.kasparov.ru"];
	
	if ((strBlocked.indexOf("http://") == -1) && (strBlocked.indexOf("https://") == -1))
		strBlocked = "http://" + strBlocked;
			
	var a = document.createElement("a");
	a.href = strBlocked;
	
	if (a.host == "kasparov.ru")
		a.host = "www."+"kasparov.ru";
	if (a.host == "grani.ru")
		a.host = "graniru.org";

	if ($("#encrypt_url").is(':checked'))
		RKN_List.push(a.host);
		
	for (n=0; n<RKN_List.length; n++)
	{
		if (a.host == RKN_List[n])
		{
			$.post("/make_short_url.ssp", { action: "make_temp", long: a.href, alias: "" }, 
						function(data, status)
						{
							if (data.result != true || data.short.length == 0)
								return;
								
							window.open("http://"+data.short[0]+"."+GetMyDomain());
						}, "json")
						.fail(function(xhr, textStatus, error) 
						{
							alert("Unknown server error!");
						});
			return;
		}
	}
	
//	deleteAllCookies(a.host + ".");
		
	window.open(a.protocol + "//" + a.host + "."+GetMyDomain() + a.pathname + a.search + a.hash);


}

