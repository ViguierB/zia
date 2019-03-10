let myapp;
$('document').ready(() => {
	myapp = (function ()  {
		let my_axios = axios.create({
			baseURL: '/',
			timeout: 10000,
			headers: {
				'manager-api': 'true'
			}
		});

		let hashPassword = (password) => {
			let hash = sha256.create();
			hash.update(password);
			return hash.hex();
		}

		let content = $("#content");
		let contentList = $("#content ul")
		let badPasswordContent = $("#bad-password");
		let passwordInput = $('#password-input__')
		let submitPassword = $('#submit-password')
		let moduleList = [];
		let password = "";

		let updateList = async () => {
			try {
				let result = await my_axios.post('/',{
					'password': hashPassword(password),
					'command': 'list'
				})

				makeListElem = ({ name, badges }) => {
					let i = moduleList.push({
						name: name,
						elem: (() => {
							let bs = badges || [];
							let _badges = "";
							bs.forEach(badge => { _badges += `<span class="badge badge-info m_badge float-right">${badge}</span>` });
							return $(`<li class="list-group-item">${name}${_badges}</li>`)
						})(),
						checked: false
					}) - 1;
					let elem = moduleList[i].elem;
					elem.on("click", () => {
						moduleList[i].checked = !moduleList[i].checked;
						if (moduleList[i].checked) {
							elem.addClass("active");
						} else {
							elem.removeClass("active");
						}
					});

					return elem;
				}
				if (Array.isArray(result.data)) {
					moduleList = [];
					contentList.empty();
					result.data.forEach((elem) => {
						contentList.append(makeListElem(elem));
					})
				}
			} catch(e) {
				console.error(e);
				throw e;
			}
			return null
		};

		let counterEnabled = false;
		let update = async () => {
			try {
				await updateList();
				badPasswordContent.css( "display", "none" );
				content.css( "display", "block" );
				counterEnabled = true;
				let getCount = async () => {
					try {
						let data = await my_axios.post('/',{
							'password': hashPassword(password),
							'command': 'counter',
						});
						
						$('#counter').text("Request count: " + data.data.count);

						if (counterEnabled)
							setTimeout(() => { getCount() }, 5000);
						return;
					} catch (e) {}
					counterEnabled = false;
				};
				getCount()
				return;
			} catch (e) {
				badPasswordContent.css( "display", "block" );
				content.css( "display", "none" );
				counterEnabled = false;
				alert("You are unauthorized to access this page");
			}
		}

		let removeOne = async (name) => (
			await my_axios.post('/',{
				'password': hashPassword(password),
				'command': 'unload',
				'module-name': name
			})
		)

		submitPassword.on('click', () => {
			update().then(() => {});
		});

		$("#reload-button").on('click', () => {
			update().then(() => {});
		});

		$("#remove-button").on('click', () => {
			let toRemove = [];
			moduleList.forEach((elem) => {
				if (elem.checked === true)
					toRemove.push(removeOne(elem.name));
			})

			Promise.all(toRemove).then(() => {
				update().then(() => {});
			})
		});

		$("#add-button").on('click', () => {
			let modulePath = prompt("Module path on the server", "/etc/zia/module.so");

			my_axios.post('/',{
				'password': hashPassword(password),
				'command': 'load',
				'module-path': modulePath
			}).then((e) => {
				if (e.data.status !== "success") {
					alert(e.data.message);
				} else {
					update().then(() => {});
				}
			});
		});

		$("#server-refresh-button").on('click', () => {
			my_axios.post('/',{
				'password': hashPassword(password),
				'command': 'refresh'
			}).then((e) => {
				if (e.data.status !== "success") {
					alert(e.data.message);
				} else {
					setTimeout(() => { update().then(() => {}); }, 2000);
				}
			});
		});
		
		passwordInput.on('input', () => {
			password = passwordInput.val();
		});

		passwordInput.on('keyup', (e) => {
			if (e.keyCode === 13) {
				event.preventDefault();

    			submitPassword.click();
			}
		});

		passwordInput.focus();
	
		
		return {};
	})();
});
