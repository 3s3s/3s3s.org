
function deleteAllCookies(strDot) {
    var cookies = document.cookie.split(";");

        for (var i = 0; i < cookies.length; i++) {
			var cookie = cookies[i];
			var eqPos = cookie.indexOf("=");
			var name = eqPos > -1 ? cookie.substr(0, eqPos) : cookie;
			document.cookie = name + "=; path=/; domain="+strDot+"3s3s.org; expires=Thu, 01 Jan 1970 00:00:00 GMT";
        }
}


$(function()
{
	/*if (window.location.hostname == "www.3s3s.org")
	{
		window.location.href = window.location.protocol + "//" + "3s3s.org" + window.location.pathname
	}*/
	deleteAllCookies("");

	$( "#accordion" ).accordion({ header: "h2", heightStyle: "content" });
	
	$( "#tabs-top" ).tabs();
	
	$("#blocked_url").bind('keypress', function(e) {
	  if (e.which != 13) {
		   return;
	  }
	  UnblockURL($("#blocked_url").val());
	});
	
	//$( "#LeftTabs").tabs().addClass( "ui-tabs-vertical ui-helper-clearfix" );
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
								
							window.open("http://"+data.short[0]+".3s3s.org");
						}, "json")
						.fail(function(xhr, textStatus, error) 
						{
							alert("Unknown server error!");
						});
			return;
		}
	}
	
//	deleteAllCookies(a.host + ".");
		
	window.open(a.protocol + "//" + a.host + ".3s3s.org" + a.pathname + a.search + a.hash);


}

$.getScript("/jscripts/utils.js", function(){
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
				for (i=0; i<data.short.length; i++)
				{
					rows += "<tr><td>http://" + data.short[i] + ".3s3s.org</td></tr>";
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
									"Sorry. Only one domain name in 24 hours available."+
								"</span><br><span>To get a short link without domain, please clean the alias field.<span>"+
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
})