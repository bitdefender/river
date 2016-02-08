(function() {
    "use strict";
    var p = true,
        W = window,
        D = document,
        $ = W.jQuery,
        B = $("body"),
        _checkDomain = function() {
            /*if (W.location.href.indexOf("ribbonjs.com") === -1 && p) {
                return true
            }*/
            return false;
        },
        _setTrialLimitations = function(a) {
            if (!_checkDomain()) {
                return
            }
            var b = 10,
                rand = W.Math.floor((W.Math.random() * b) + 1),
                bBox = a.getBoundingBox(),
                limitations = 'If you wish to remove all trial limitations and messages and use the control on a live server you should purchase a license from http://ribbonjs.com/buy.',
                messages = ['The enableRibbon() method of Ribbon JS has been disabled for the trial version. ' + limitations, '<p title="The trial version of this control is for personal and testing purposes only. If you want to use AcidJs.Ribbon on a live server, you should purchase the full version.">This is a trial and limited version of <a href="http://ribbonjs.com/" target="_blank">AcidJs.Ribbon</a>. Please, <a href="http://ribbonjs.com/buy">purchase</a> the full version.</p>', 'You are using a trial version of Ribbon JS, which is being disabled randomly. ' + limitations, 'Please, reload the page and try again.'];
            W.console.info(messages[2]);
            bBox.append(messages[1]);
            if ([0, 3, 9, 6, 2].indexOf(rand) !== -1) {
                this.enableRibbon = function() {
                    W.console.info(messages[0])
                };
                this._later(function() {
                    W.console.info(messages[2], messages[3]);
                    a.disableRibbon()
                }, 2000)
            }
        };
    if (undefined === W.AcidJs) {
        W.AcidJs = {}
    }

    function Ribbon(a) {
        a = a || {};
        if (!a.boundingBox || !a.boundingBox.length) {
            return W.console.error(this.MESSAGES.boundingBoxIsNull)
        }
        for (var b in a) {
            if (a.hasOwnProperty(b)) {
                this[b] = a[b]
            }
        }
        if (this.loadCss) {
            this._loadStylesheet("base")
        }
    }
    Ribbon.prototype = {
        _enabled: true,
        MANIFEST: {
            version: "4.3.0",
            name: "Ribbon JS",
            releaseDate: "2015-03-28",
            releaseNotes: "http://ribbonjs.com/release-notes",
            developer: "Martin Ivanov",
            description: "Web-based MSOffice-like ribbon toolbar, built with HTML5, JavaScript and CSS3",
            websites: {
                page: "http://ribbonjs.com/",
                portfolio: "http://wemakesites.net",
                blog: "http://martinivanov.net/",
                twitter: "https://twitter.com/wemakesitesnet",
                purchase: "http://ribbonjs.com/buy"
            },
            email: "acid_martin@yahoo.com"
        },
        props: {},
        tabProps: {},
        highLightedTabs: {},
        cssClasses: [],
        loadCss: true,
        ATTRS: {
            name: "data-name",
            value: "data-value",
            type: "data-type",
            toolName: "data-tool-name",
            open: "open",
            hidden: "hidden",
            size: "data-tool-size",
            tabGroupName: "data-tab-group-name"
        },
        EVENTS: ["acidjs-ribbon-tool-clicked", "acidjs-ribbon-ready", "acidjs-ribbon-tab-changed", "acidjs-ribbon-toggle"],
        URLS: {
            base: "AcidJs.Ribbon/styles/Ribbon.css",
            icon: "{appRoot}AcidJs.Ribbon/icons/{size}/{name}.png"
        },
        appRoot: "",
        width: "100%",
        MESSAGES: {
            boundingBoxIsNull: "Cannot render ribbon, because the specified config.boundingBox is null or undefined."
        },
        CSS_CLASSES: {
            ui: "acidjs-ui",
            base: "acidjs-ui-ribbon",
            flat: "acidjs-ui-ribbon-flat",
            ribbonDropdown: "acidjs-ui-ribbon-dropdown",
            open: "acidjs-ui-ribbon-dropdown-open",
            tabButtons: "acidjs-ui-ribbon-tab-buttons",
            ribbons: "acidjs-ui-ribbon-tab-ribbons",
            tabContent: "acidjs-ui-ribbon-tab-content",
            selected: "acidjs-ui-ribbon-tool-selected",
            dropdown: "acidjs-ui-ribbon-tool-dropdown",
            colorPicker: "acidjs-ui-ribbon-tool-color-picker",
            colorPickerDropdownArrow: ".acidjs-ui-ribbon-tool-color-picker > div:first-child .acidjs-ribbon-arrow",
            splitButton: "acidjs-ui-ribbon-tool-split-button",
            dropdownArrow: ".acidjs-ui-ribbon-tool-dropdown .acidjs-ribbon-arrow",
            splitButtonArrow: ".acidjs-ui-ribbon-tool-split-button .acidjs-ribbon-arrow",
            buttonsExclusive: "acidjs-ui-ribbon-tool-exclusive-buttons",
            toggleButtons: "acidjs-ui-ribbon-tool-toggle-buttons",
            active: "acidjs-ui-tool-active",
            quickLaunch: "acidjs-ui-ribbon-quick-launch",
            fileTabMenuPlaceholder: "acidjs-ui-ribbon-file-tab-menu",
            quickLaunchEnabled: "acidjs-ui-ribbon-quick-launch-enabled",
            tabGroup: "acidjs-ui-tabgroup"
        },
        TEMPLATES: {
            base: ['<div>', '<div class="ribbon-whitespace">', '<button type="button" class="no-drag" name="toggle-ribbon"></button>', '<div class="acidjs-ui-ribbon-quick-launch no-drag"></div>', '<div class="acidjs-ui-ribbon-file-tab-menu no-drag"></div>', '<ul class="acidjs-ui-ribbon-tab-buttons no-drag"></ul>', '</div>', '<div class="acidjs-ui-ribbon-tabs">', '<ul class="acidjs-ui-ribbon-tab-content"></ul>', '</div>', '</div>'],
            tabGroup: ['<div data-tab-group-name="<#= groupName #>" class="acidjs-ui-tabgroup" style="top: <#= top #>px; left: <#= left #>px; width: <#= width #>px; background: <#= color #>;">', '<span style="border-top-color: <#= color #>;"><#= groupTitle #></span>', '</div>'],
            tabButton: ['<li>', '<a <#= ng #> data-name="<#= name #>" <#= visible #> <#= enabled #> href="#" title="<#= hint #>"><#= label #></a>', '</li>'],
            tabContent: ['<li>', '<div data-name="<#= name #>" <#= visible #> <#= enabled #> class="acidjs-ui-ribbon-tab-ribbons">', '<ul>', '<#= ribbonsHtml #>', '</ul>', '</div>', '</li>'],
            ribbon: ['<li <#= ng #> data-label="<#= label || "&nbsp;" #>" style="width: <#= width #>; min-width: <#= minWidth #>;">', '<div class="acidjs-ui-ribbon-tab-ribbon-tools">', '<#= toolsHtml #>', '</div>', '<h6>', '<#= label || "&nbsp;" #>', '</h6>', '</li>'],
            buttons: ['<ul data-tool="<#= type #>" data-items="<#= items #>" class="acidjs-ui-ribbon-tool-strip acidjs-ui-ribbon-tool-<#= type #>" data-size="<#= size #>">', '<# var icons = size === "large" ? "large" : "small"; #>', '<# if(commands && commands.length) { #>', '<# for(var i = 0; i < commands.length; i ++) { #>', '<# var command = commands[i]; #>', '<li>', '<a class="acidjs-ui-ribbon-tool" href="#" data-tool-name="<#= command.name #>" name="<#= command.name #>" title="<#= command.hint #>">', '<# if(command.icon) { #>', '<img width="<#= size === "large" ? 32 : 16 #>" height="<#= size === "large" ? 32 : 16 #>" src="<#= appRoot #>AcidJs.Ribbon/icons/<#= icons #>/<#= command.icon #>" />', '<# } #>', '<# if(command.label) { #>', '<span><#= command.label #></span>', '<# } #>', '</a>', '</li>', '<# } #>', '<# } #>', '</ul>'],
            dropdown: ['<# var items = items || [], width = width ? width + "px;" : "43px"; #>', '<# var selected = "acidjs-ui-ribbon-tool-selected"; #>', '<# if(items.length) { #>', '<div data-tool="dropdown" data-tool-name="<#= name #>" class="acidjs-ui-ribbon-tool-dropdown" style="width: <#= width #>">', '<div>', '<div>', '<a href="#" name="<#= name #>" data-value="<#= items[0].value || "" #>"><#= items[0].label || "" #></a>', '<strong class="acidjs-ribbon-arrow"></strong>', '</div>', '</div>', '<div class="acidjs-ui-ribbon-dropdown" style="width: <#= width #>">', '<ul>', '<# for(var i = 0; i < items.length; i ++) { #>', '<# var item = items[i]; #>', '<li>', '<a class="<#= i === 0 ? selected : "" #>" href="#" name="<#= name #>" data-value="<#= item.value || "" #>">', '<span><#= item.label || "" #></span>', '</a>', '</li>', '<# } #>', '</ul>', '</div>', '</div>', '<# } #>'],
            toolBreak: ['<div data-tool="break" class="acidjs-ui-ribbon-tool-break"></div>'],
            separator: ['<div data-tool="separator" class="acidjs-ui-ribbon-tool-separator"></div>'],
            colorPicker: ['<div data-tool-name="<#= name #>" data-tool="color-picker" class="acidjs-ui-ribbon-tool-color-picker">', '<div>', '<# var hint = hint || ""; #>', '<# var label = label || "&nbsp;"; #>', '<a href="#" data-value="<#= colors[0] #>" data-type="<#= type #>" name="<#= name #>" title="<#= hint #>">', '<span>', '<img width="14" height="14" src="<#= appRoot + "AcidJs.Ribbon/icons/small/" + icon #>" />', '<span class="acidjs-ui-ribbon-color-preview" style="background: <#= colors[0] #>"></span>', '</span>', '<span>', '<#= label #>', '</span>', '</a>', '<# if(colors.length > 1) { #>', '<strong class="acidjs-ribbon-arrow"><!-- / --></strong>', '<# } #>', '</div>', '<# if(colors && colors.length) { #>', '<# var dropdownWidth = dropdownWidth || ""; #>', '<# var selected = "acidjs-ui-ribbon-tool-selected"; #>', '<div class="acidjs-ui-ribbon-dropdown" style="width: <#= dropdownWidth #>px;">', '<ul>', '<# for(var i = 0; i < colors.length; i ++) { #>', '<# var color = colors[i]; #>', '<li>', '<a class="<#= i === 0 ? selected : "" #>" style="background: <#= color #>;" title="<#= color #>" href="#" name="<#= name #>" data-value="<#= color #>"><#= color #></a>', '</li>', '<# } #>', '</ul>', '</div>', '</div>', '<# } #>'],
            genericDropdown: ['<# var dropdownContent = dropdownContent || ""; #>', '<# var label = label || ""; #>', '<# var icon = icon || null; #>', '<# var hint = hint || ""; #>', '<div data-tool-name="<#= name #>" data-tool="<#= type #>" class="acidjs-ribbon-generic-dropdown">', '<div>', '<a href="#" title="<#= hint || "" #>" data-type="<#= type #>" name="<#= name #>">', '<# if(icon) { #>', '<img width="16" height="16" src="<#= appRoot + "AcidJs.Ribbon/icons/small/" + icon #>" />', '<# } #>', '<span>', '<#= label #>', '</span>', '<strong class="acidjs-ribbon-arrow"><!-- / --></strong>', '</a>', '</div>', '<div class="acidjs-ui-ribbon-dropdown"><#= dropdownContent #></div>', '</div>'],
            toggleMenuItems: ['<# var items = items || []; #>', '<# if(items.length) { #>', '<div class="acidjs-ui-ribbon-toggle-menu-items">', '<form>', '<ul>', '<# for(var i = 0; i < items.length; i ++) { #>', '<# var item = items[i]; #>', '<li>', '<label class="acidjs-ui-ribbon-tool-checkbox-label">', '<input type="checkbox" value="<#= item.value ? item.value : "null" #>" <#= item.selected ? "checked" : "" #> name="<#= item.name #>" />', '<span href="#"><#= item.label #></span>', '</label>', '</li>', '<# } #>', '</ul>', '</form>', '</div>', '<# } #>'],
            exclusiveMenuItems: ['<# var items = items || []; #>', '<# var defaultSelectedItem = defaultSelectedItem || 0; #>', '<# if(items.length) { #>', '<div class="acidjs-ui-ribbon-exclusive-menu-items">', '<form>', '<ul>', '<# for(var i = 0; i < items.length; i ++) { #>', '<# var item = items[i]; #>', '<li>', '<label class="acidjs-ui-ribbon-tool-checkbox-label" name="<#= name #>">', '<input <#= defaultSelectedItem === i ? "checked" : "" #> type="radio" name="<#= name #>" value="<#= item.value #>" />', '<span><#= item.label #></span>', '</label>', '</li>', '<# } #>', '</ul>', '</form>', '</div>', '<# } #>'],
            checkbox: ['<# var value = value || ""; #>', '<# var hint = hint || ""; #>', '<# var checked = checked ? "checked" : ""; #>', '<div class="acidjs-ui-ribbon-tool-checkbox" data-tool-name="<#= name #>" data-tool-type="checkbox" title="<#= hint #>">', '<label class="acidjs-ui-ribbon-tool-checkbox-label">', '<input <#= checked #> type="checkbox" value="<#= value #>" name="<#= name #>" />', '<span><#= label #></span>', '</label>', '</div>'],
            radios: ['<# var items = items || ""; #>', '<# var defaultSelectedItem = defaultSelectedItem || 0; #>', '<div class="acidjs-ui-ribbon-tool-radios" data-tool-name="<#= name #>" data-tool-type="radios">', '<form>', '<ul>', '<# for(var i = 0; i < items.length; i ++) { #>', '<# var item = items[i]; #>', '<# var value = item.value || ""; #>', '<li>', '<label class="acidjs-ui-ribbon-tool-checkbox-label" name="<#= name #>">', '<input <#= defaultSelectedItem === i ? "checked" : "" #> type="radio" name="<#= name #>" value="<#= item.value #>" />', '<span><#= item.label #></span>', '</label>', '</li>', '<# } #>', '</ul>', '</form>', '</div>'],
            splitButton: ['<# var commands = commands || []; #>', '<# if(commands.length) { #>', '<div data-tool-name="<#= commands[0].name #>" data-tool-size="<#= size || "large" #>" data-tool="<#= type #>" class="acidjs-ui-ribbon-tool-split-button acidjs-ui-ribbon-tool-split-button-<#= size || "large" #>">', '<# var hint = commands[0].hint || ""; #>', '<div title="<#= hint #>">', '<a href="#" data-type="<#= type #>" name="<#= commands[0].name #>">', '<# var headerIcon = commands[0].icon ? appRoot + "AcidJs.Ribbon/icons/" + size + "/" + commands[0].icon : appRoot + "AcidJs.Ribbon/icons/shim.png"; #>', '<img width="<#= size === "large" ? 32 : 16 #>" height="<#= size === "large" ? 32 : 16 #>" src="<#= headerIcon #>" />', '<span>', '<#= commands[0].label || "" #>', '</span>', '<# if(commands.length > 1) { #>', '<strong class="acidjs-ribbon-arrow"><!-- / --></strong>', '<# } #>', '</a>', '</div>', '<# if(commands.length > 1) { #>', '<div class="acidjs-ui-ribbon-dropdown">', '<# var selected = "acidjs-ui-ribbon-tool-selected"; #>', '<ul>', '<# for(var i = 0; i < commands.length; i ++) { #>', '<# var command = commands[i]; #>', '<li>', '<a data-tool-name="<#= command.name #>" class="<#= i === 0 ? selected : "" #>" href="#" name="<#= command.name #>">', '<# var icon = command.icon ? appRoot + "AcidJs.Ribbon/icons/small/" + command.icon : appRoot + "AcidJs.Ribbon/icons/shim.png"; #>', '<img src="<#= icon #>" />', '<# if(command.label) { #>', '<span>', '<#= command.label || "" #>', '</span>', '<# } #>', '</a>', '</li>', '<# } #>', '</ul>', '</div>', '<# } #>', '</div>', '<# } #>']
        },
        getBoundingBox: function() {
            return this.boundingBox
        },
        getConfig: function() {
            return this.config
        },
        destroy: function() {
            var a = this.getBoundingBox(),
                config = this.getConfig();
            a.removeAttr("class").removeAttr("style").empty();
            delete this.config;
            return config
        },
        disableTools: function(a) {
            if (!a || !a.length) {
                return
            }
            $.each(a, $.proxy(function(i) {
                this.getToolsByName(a[i]).attr("disabled", "")
            }, this))
        },
        selectDropdownValue: function(a, b, c) {
            if (!a && !b) {
                return
            }
            var d = this.getToolsByName(a),
                valueAttr = this.ATTRS.value,
                selected = this.CSS_CLASSES.selected,
                newItem = d.find('.acidjs-ui-ribbon-dropdown a[data-value="' + b + '"]'),
                newItemValue = newItem.attr(valueAttr),
                toolHeader = d.find(' > div:first-child a');
            toolHeader.attr(valueAttr, newItemValue).html(newItem.find("span").text());
            d.find("." + selected).removeClass(selected);
            newItem.addClass(selected);
            if (c) {
                newItem.trigger("click")
            }
        },
        selectColorByValue: function(a, b, c) {
            if (!a && !b) {
                return
            }
            var d = this.getToolsByName(a),
                valueAttr = this.ATTRS.value,
                selected = this.CSS_CLASSES.selected,
                newItem = d.find('.acidjs-ui-ribbon-dropdown a[data-value="' + b + '"]'),
                newItemValue = newItem.attr(valueAttr),
                toolHeader = d.find(' > div:first-child a'),
                toolHeaderColorPreview = toolHeader.find(".acidjs-ui-ribbon-color-preview");
            toolHeader.attr(valueAttr, newItemValue);
            toolHeaderColorPreview.css({
                background: newItemValue
            });
            d.find("." + selected).removeClass(selected);
            newItem.addClass(selected);
            if (c) {
                newItem.trigger("click")
            }
        },
        enableTools: function(a) {
            if (!a || !a.length) {
                return
            }
            $.each(a, $.proxy(function(i) {
                this.getToolsByName(a[i]).removeAttr("disabled")
            }, this))
        },
        setToolsActive: function(b, c) {
            if (!b || !b.length) {
                return
            }
            $.each(b, $.proxy(function(i) {
                var a = this.getToolsByName(b[i]);
                a.addClass(this.CSS_CLASSES.active);
                if (c) {
                    a.trigger("click");
                    a.addClass(this.CSS_CLASSES.active)
                }
            }, this))
        },
        setToolsInactive: function(a) {
            if (!a || !a.length) {
                return
            }
            $.each(a, $.proxy(function(i) {
                this.getToolsByName(a[i]).removeClass(this.CSS_CLASSES.active)
            }, this))
        },
        clickTools: function(a) {
            if (!a || !a.length) {
                return
            }
            $.each(a, $.proxy(function(i) {
                this.getBoundingBox().find('[' + this.ATTRS.toolName + '="' + a[i] + '"]:eq(0)').trigger("click")
            }, this))
        },
        getToolData: function(a) {
            var b = $(this.getBoundingBox().find('[name="' + a + '"]').get(0)),
                data = {
                    props: this.props[a] || null,
                    command: b.attr("name") || null,
                    active: b.is("." + this.CSS_CLASSES.active) ? true : false,
                    value: b.val() || null
                };
            return data
        },
        getToolProps: function(a) {
            return this.props[a] || null
        },
        getTabProps: function(a) {
            return this.tabProps[a] || null
        },
        setTabProps: function(a, b, c) {
            if (!this.tabProps[a]) {
                this.props[a] = {}
            }
            this.tabProps[a][b] = c;
            return this.getTabProps(a)
        },
        setToolProps: function(a, b, c) {
            if (!this.props[a]) {
                this.props[a] = {}
            }
            this.props[a][b] = c;
            return this.getToolProps(a)
        },
        getReleaseNotes: function() {
            W.open(this.MANIFEST.releaseNotes, "_blank")
        },
        getToolsByName: function(a) {
            return this.getBoundingBox().find('[' + this.ATTRS.toolName + '="' + a + '"]')
        },
        disableRibbon: function() {
            this.getBoundingBox().attr("disabled", "");
            this._enabled = false
        },
        enableRibbon: function() {
            this.getBoundingBox().removeAttr("disabled");
            this._enabled = true
        },
        hide: function() {
            this.getBoundingBox().attr(this.ATTRS.hidden, "")
        },
        show: function() {
            this.getBoundingBox().removeAttr(this.ATTRS.hidden)
        },
        expand: function() {
            this.getBoundingBox().attr(this.ATTRS.open, "").trigger(this.EVENTS[3], {
                expanded: true
            })
        },
        collapse: function() {
            this._later($.proxy(function() {
                this.getBoundingBox().removeAttr(this.ATTRS.open).trigger(this.EVENTS[3], {
                    expanded: false
                })
            }, this), 100)
        },
        enableFlatStyles: function() {
            this.flat = true;
            this.getBoundingBox().addClass(this.CSS_CLASSES.flat);
            return this.flat
        },
        disableFlatStyles: function() {
            this.flat = false;
            this.getBoundingBox().removeClass(this.CSS_CLASSES.flat);
            return this.flat
        },
        highlightTabsGroup: function(a, b, c, d) {
            if (!a || !a.length || !b || !c || !d) {
                return
            }
            var e = this.getBoundingBox(),
                firstTab = this.getTabButtonByName(a[0]).parent(),
                offsetTop = firstTab.height() * -1,
                classes = this.CSS_CLASSES,
                attrs = this.ATTRS,
                firstTabOffsetLeft = firstTab.position().left,
                lastTab = this.getTabButtonByName(a[a.length - 1]).parent(),
                lastTabOffsetRight = lastTab.position().left + lastTab.width(),
                tabButtonsNode = e.find("." + classes.tabButtons);
            if (e.find('[' + attrs.tabGroupName + '="' + b + '"]').length) {
                return
            }
            tabButtonsNode.prepend(this._template("tabGroup", {
                groupName: b,
                groupTitle: c,
                left: firstTabOffsetLeft,
                top: offsetTop,
                color: d,
                width: lastTabOffsetRight - firstTabOffsetLeft
            }));
            this.highLightedTabs[b] = {
                tabs: a,
                groupTitle: c,
                color: d,
                name: b
            };
            return this.highLightedTabs
        },
        unhighlightTabsGroup: function(a) {
            var b = this.highLightedTabs[a];
            if (!b || !a) {
                return
            }
            this.getBoundingBox().find('[' + this.ATTRS.tabGroupName + '="' + b.name + '"]').remove();
            delete this.highLightedTabs[a];
            return this.highLightedTabs
        },
        getTabButtonByName: function(a) {
            if (!a) {
                return
            }
            return this.getBoundingBox().find('.acidjs-ui-ribbon-tab-buttons a[data-name="' + a + '"]')
        },
        setTabActive: function(a) {
            if (!a) {
                return
            }
            this.getTabButtonByName(a).trigger("click")
        },
        disableTabs: function(b) {
            if (!b || !b.length) {
                return
            }
            $.each(b, $.proxy(function(i) {
                var a = this.getTabButtonByName(b[i]);
                if (a.is("." + this.CSS_CLASSES.selected)) {
                    return
                }
                a.attr("disabled", "")
            }, this))
        },
        enableTabs: function(b) {
            if (!b || !b.length) {
                return
            }
            $.each(b, $.proxy(function(i) {
                var a = this.getTabButtonByName(b[i]);
                if (a.is("." + this.CSS_CLASSES.selected)) {
                    return
                }
                a.removeAttr("disabled")
            }, this))
        },
        removeTabs: function(b) {
            if (!b || !b.length) {
                return
            }
            $.each(b, $.proxy(function(i) {
                var a = b[i],
                    tabButtonNode = this.getTabButtonByName(a);
                if (tabButtonNode.is("." + this.CSS_CLASSES.selected)) {
                    return
                }
                tabButtonNode.parent().remove();
                this.getTabContentBoxByName(a).parent().remove()
            }, this))
        },
        hideTabs: function(b) {
            if (!b || !b.length) {
                return
            }
            var c = this.ATTRS.hidden;
            $.each(b, $.proxy(function(i) {
                var a = b[i],
                    tabButtonNode = this.getTabButtonByName(a);
                if (tabButtonNode.is("." + this.CSS_CLASSES.selected)) {
                    return
                }
                tabButtonNode.attr(c, "");
                this.getTabContentBoxByName(a).attr(c, "")
            }, this))
        },
        getTabContentBoxByName: function(a) {
            if (!a) {
                return
            }
            return this.getBoundingBox().find('.acidjs-ui-ribbon-tab-content [' + this.ATTRS.name + '="' + a + '"]')
        },
        showTabs: function(b) {
            if (!b || !b.length) {
                return
            }
            var c = this.ATTRS.hidden;
            $.each(b, $.proxy(function(i) {
                var a = b[i],
                    tabButtonNode = this.getTabButtonByName(a);
                tabButtonNode.removeAttr(c);
                this.getTabContentBoxByName(a).removeAttr(c)
            }, this))
        },
        renderTabs: function(g, h) {
            g = g || this.config.tabs || [];
            h = h || "append";
            var o = this.getBoundingBox(),
                classes = this.CSS_CLASSES,
                tabButtons = o.find("." + classes.tabButtons),
                tabContent = o.find("." + classes.tabContent),
                tabButtonsHtml = [],
                tabContentHtml = [];
            $.each(g, $.proxy(function(i) {
                var f = g[i],
                    ng = f.ng || {},
                    guid = this._guid(),
                    ribbons = f.ribbons || [],
                    ribbonsHtml = [],
                    tabButtonsHtmlDto = {
                        label: f.label || "",
                        hint: f.hint || "",
                        name: f.name || guid,
                        visible: f.visible === false ? "hidden" : "",
                        enabled: f.enabled === false ? "disabled" : ""
                    };
                tabButtonsHtmlDto.ng = this._createNgDirectives(ng);
                this.tabProps[tabButtonsHtmlDto.name] = f.props || null;
                if ("append" === h) {
                    tabButtonsHtml.push(this._template("tabButton", tabButtonsHtmlDto))
                } else {
                    tabButtonsHtml.unshift(this._template("tabButton", tabButtonsHtmlDto))
                }
                $.each(ribbons, $.proxy(function(j) {
                    var e = ribbons[j],
                        ribbonNg = e.ng || {},
                        toolsHtml = [];
                    e.tools = e.tools || [];
                    $.each(e.tools, $.proxy(function(k) {
                        var b = e.tools[k];
                        b.appRoot = this.appRoot;
                        switch (b.type) {
                            case "buttons":
                            case "toggle-buttons":
                            case "exclusive-buttons":
                                b.items = b.items || "floated";
                                if (b.commands) {
                                    $.each(b.commands, $.proxy(function(l) {
                                        var a = b.commands[l];
                                        if (a.props) {
                                            this.props[a.name] = a.props
                                        }
                                    }, this))
                                }
                                toolsHtml.push(this._template("buttons", b));
                                break;
                            case "dropdown":
                                if (b.items) {
                                    this.props[b.name] = [];
                                    $.each(b.items, $.proxy(function(m) {
                                        var a = b.items[m];
                                        this.props[b.name].push(a.props || null)
                                    }, this))
                                }
                                toolsHtml.push(this._template("dropdown", b));
                                break;
                            case "split-button":
                            case "menu":
                                if (b.commands) {
                                    $.each(b.commands, $.proxy(function(l) {
                                        var a = b.commands[l];
                                        if (a.props) {
                                            this.props[a.name] = a.props
                                        }
                                    }, this))
                                }
                                toolsHtml.push(this._template("splitButton", b));
                                break;
                            case "checkbox":
                                this.props[b.name] = b.props || null;
                                toolsHtml.push(this._template("checkbox", b));
                                break;
                            case "radios":
                                if (b.items) {
                                    this.props[b.name] = [];
                                    $.each(b.items, $.proxy(function(m) {
                                        var a = b.items[m];
                                        this.props[b.name].push(a.props || null)
                                    }, this))
                                }
                                toolsHtml.push(this._template("radios", b));
                                break;
                            case "break":
                                toolsHtml.push(this._template("toolBreak", {}));
                                break;
                            case "separator":
                                toolsHtml.push(this._template("separator", {}));
                                break;
                            case "custom":
                                if (b.templateId && b.data) {
                                    var c = $("#" + b.templateId);
                                    this._setTemplate(b.templateId, c.html());
                                    toolsHtml.push(this._template(b.templateId, b.data))
                                }
                                break;
                            case "color-picker":
                                this.props[b.name] = b.props || null;
                                toolsHtml.push(this._template("colorPicker", b));
                                break;
                            case "custom-dropdown":
                                if (b.templateId && b.data) {
                                    var d = $("#" + b.templateId);
                                    this._setTemplate(b.templateId, d.html());
                                    b.dropdownContent = this._template(b.templateId, b.data);
                                    toolsHtml.push(this._template("genericDropdown", b))
                                }
                                break;
                            case "toggle-dropdown":
                                if (b.items) {
                                    this.props[b.name] = [];
                                    $.each(b.items, $.proxy(function(n) {
                                        var a = b.items[n];
                                        this.props[b.name].push(a.props || null)
                                    }, this))
                                }
                                b.dropdownContent = this._template("toggleMenuItems", b);
                                toolsHtml.push(this._template("genericDropdown", b));
                                break;
                            case "exclusive-dropdown":
                                if (b.items) {
                                    this.props[b.name] = [];
                                    $.each(b.items, $.proxy(function(n) {
                                        var a = b.items[n];
                                        this.props[b.name].push(a.props || null)
                                    }, this))
                                }
                                b.dropdownContent = this._template("exclusiveMenuItems", b);
                                toolsHtml.push(this._template("genericDropdown", b));
                                break
                        }
                    }, this));
                    e.toolsHtml = toolsHtml.join("");
                    e.width = e.width || "auto";
                    e.minWidth = e.minWidth || "auto";
                    e.ng = this._createNgDirectives(ribbonNg);
                    ribbonsHtml.push(this._template("ribbon", e))
                }, this));
                tabContentHtml.push(this._template("tabContent", {
                    ribbonsHtml: ribbonsHtml.join(""),
                    name: f.name || guid,
                    visible: f.visible === false ? "hidden" : "",
                    enabled: f.visible === false ? "hidden" : ""
                }))
            }, this));
            tabButtons[h](tabButtonsHtml.join(""));
            tabContent[h](tabContentHtml)
        },
        init: function() {
            if (this.ready) {
                return
            }
            var c = this.boundingBox,
                classes = this.CSS_CLASSES,
                base = classes.base,
                cssClasses = [classes.ui, base],
                ng = this.ng;
            if (this.flat) {
                cssClasses.push(classes.flat)
            }
            if (ng) {
                $.each(ng, function(a, b) {
                    c.attr(a.indexOf("data-") > -1 ? a : "ng-" + a, b)
                })
            }
            cssClasses = cssClasses.concat(this.cssClasses || []);
            c.addClass(cssClasses.join(" ")).attr("open", "").html(this._template("base")).css({
                maxWidth: this.width,
                minWidth: this.minwidth,
                visibility: "hidden"
            });
            this.renderTabs();
            this.renderQuickLaunchToolbar();
            this.renderFileTabMenu();
            this._bind()
        },
        renderFileTabMenu: function(a) {
            var b = this.getBoundingBox().find("." + this.CSS_CLASSES.fileTabMenuPlaceholder),
                tool = {
                    type: "menu",
                    commands: a || this.config.fileTabMenu || [],
                    size: "small",
                    appRoot: this.appRoot
                };
            b.html(this._template("splitButton", tool))
        },
        renderQuickLaunchToolbar: function(a) {
            var b = this.getBoundingBox(),
                classes = this.CSS_CLASSES,
                node = this.getBoundingBox().find("." + classes.quickLaunch),
                tool = {
                    type: "buttons",
                    commands: a || this.config.quickLaunchToolbar || [],
                    items: "floated",
                    size: "small",
                    appRoot: this.appRoot
                };
            tool.commands.unshift({
                name: "acidjs-ui-ribbon-app-icon",
                icon: this.appIconUrl || "acidjs-ui-ribbon-app-icon.png"
            });
            b.addClass(classes.quickLaunchEnabled);
            node.html(this._template("buttons", tool))
        },
        getRibbonByLabel: function(a) {
            if (!a) {
                return
            }
            return this.getBoundingBox().find('.acidjs-ui-ribbon-tabs [data-label="' + a + '"]')
        },
        getRibbonContextBoxByLabel: function(a) {
            if (!a) {
                return
            }
            return this.getRibbonByLabel(a).find('.acidjs-ui-ribbon-tab-ribbon-tools')
        },
        selectSplitButtonCommand: function(a, b) {
            if (!a) {
                return
            }
            var c = this.getBoundingBox(),
                selected = this.CSS_CLASSES.selected,
                button = c.find('.acidjs-ui-ribbon-tool-split-button:not([data-tool="menu"]) .acidjs-ui-ribbon-dropdown a[name="' + a + '"]'),
                buttonParent = button.parents(".acidjs-ui-ribbon-tool-split-button"),
                dropdown = buttonParent.find(".acidjs-ui-ribbon-dropdown"),
                buttonHeader = buttonParent.find(" > div:first-child a"),
                buttonHeaderIcon = buttonHeader.find("img"),
                buttonHeaderLabel = buttonHeader.find("span"),
                toolSize = buttonParent.attr(this.ATTRS.size),
                newButtonLabel = button.find("span"),
                newButtonIcon = button.find("img").attr("src"),
                newButtonCommand = button.attr("name");
            if (!button.length) {
                return
            }
            if (b) {
                return button.trigger("click")
            }
            if ("large" === toolSize) {
                newButtonIcon = newButtonIcon.replace("/small/", "/large/")
            }
            if (newButtonIcon.indexOf("shim.png")) {
                buttonHeaderIcon.attr("src", newButtonIcon)
            }
            buttonHeader.attr("name", newButtonCommand);
            buttonParent.attr(this.ATTRS.toolName, newButtonCommand);
            buttonHeaderLabel.html(newButtonLabel.html());
            dropdown.find("." + selected).removeClass(selected);
            button.addClass(selected)
        },
        _later: function(a, b) {
            W.setTimeout(function() {
                a.call(this)
            }, b || 4)
        },
        _guid: function() {
            var d = new Date().getTime(),
                uuid = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx".replace(/[xy]/g, function(c) {
                    var r = (d + W.Math.random() * 16) % 16 | 0;
                    d = W.Math.floor(d / 16);
                    return (c === "x" ? r : (r & 0x7 | 0x8)).toString(16)
                });
            return uuid
        },
        _template: function(a, b) {
            if (!this.VIEWS) {
                this.VIEWS = {}
            }
            if (this.TEMPLATES[a] instanceof Array) {
                this.TEMPLATES[a] = this.TEMPLATES[a].join("")
            }
            var c = !/\W/.test(a) ? this.VIEWS[a] = this.VIEWS[a] || this._template(this.TEMPLATES[a]) : new W.Function("obj", "var p = [], print = function() {p.push.apply(p, arguments);};" + "with(obj) {p.push('" + a.replace(/[\r\t\n]/g, " ").split("<#").join("\t").replace(/((^|#>)[^\t]*)'/g, "$\r").replace(/\t=(.*?)#>/g, "',$1,'").split("\t").join("');").split("#>").join("p.push('").split("\r").join("\\'") + "');} return p.join('');");
            return b || "" ? c(b) : c
        },
        _halt: function(a) {
            a.preventDefault();
            a.stopPropagation()
        },
        _loadStylesheet: function(a) {
            var b = D.createElement("link"),
                id = this.CSS_CLASSES.base + "-" + a.toLowerCase() + "-css";
            b.setAttribute("rel", "stylesheet");
            b.setAttribute("href", this.appRoot + this.URLS[a]);
            b.setAttribute("id", id);
            if ($("#" + id).length <= 0) {
                D.getElementsByTagName("head")[0].appendChild(b)
            }
        },
        _bind: function() {
            var c = this.CSS_CLASSES,
                events = this.EVENTS,
                attrs = this.ATTRS,
                active = c.active,
                selected = c.selected,
                ribbons = c.ribbons,
                bBox = this.getBoundingBox();
            bBox.delegate("." + c.tabGroup, "click", $.proxy(function(e) {
                var a = $(e.currentTarget),
                    tabGroupData = this.highLightedTabs[a.attr(attrs.tabGroupName)];
                this.setTabActive(tabGroupData.tabs[0])
            }, this));
            bBox.delegate('button[name="toggle-ribbon"]', "click", $.proxy(function() {
                if (bBox.is("[open]")) {
                    return this.collapse()
                }
                this.expand()
            }, this));
            bBox.delegate('.acidjs-ui-ribbon-tool-split-button .acidjs-ui-ribbon-dropdown a[name]', "click", function() {
                var a = $(this),
                    parent = a.parents('.acidjs-ui-ribbon-tool-split-button'),
                    toolHeader = parent.find(" > div:first-child a"),
                    toolHeaderIcon = toolHeader.find("img"),
                    toolHeaderLabel = toolHeader.find("span"),
                    name = a.attr("name"),
                    toolSize = parent.attr("data-tool-size"),
                    label = a.text(),
                    toolType = parent.attr("data-tool"),
                    icon = a.find("img").attr("src");
                if ("large" === toolSize && "menu" !== toolType) {
                    icon = icon.replace("/small/", "/large/")
                }
                if ("menu" !== toolType) {
                    toolHeaderLabel.html(label)
                }
                if (icon.indexOf("shim.png") === -1 && "menu" !== toolType) {
                    toolHeaderIcon.attr("src", icon)
                }
                toolHeader.attr("name", name);
                parent.attr("data-tool-name", name);
                parent.find("." + selected).removeClass(selected);
                a.addClass(selected)
            });
            bBox.delegate('[data-tool="dropdown"] .acidjs-ui-ribbon-dropdown a[name]', "click", function() {
                var a = $(this),
                    parent = a.parents('[data-tool="dropdown"]'),
                    toolHeader = parent.find(" > div:first-child a"),
                    value = a.attr(attrs.value),
                    label = a.text();
                toolHeader.attr(attrs.value, value).html(label);
                parent.find("." + selected).removeClass(selected);
                a.addClass(selected)
            });
            bBox.delegate('[data-tool="color-picker"] li a[name]', "click", function() {
                var a = $(this),
                    parent = a.parents('[data-tool="color-picker"]'),
                    colorPreview = parent.find(".acidjs-ui-ribbon-color-preview"),
                    toolHeader = parent.find(" > div:first-child a"),
                    color = a.attr(attrs.value);
                colorPreview.css({
                    background: color
                });
                toolHeader.attr(attrs.value, color);
                parent.find("." + selected).removeClass(selected);
                a.addClass(selected)
            });
            bBox.delegate("." + c.tabButtons + " a", "click", $.proxy(function(e) {
                e.preventDefault();
                var a = $(e.currentTarget),
                    tabName = a.attr(attrs.name),
                    tabButtons = a.parents("ul");
                tabButtons.find("." + selected).removeClass(selected);
                a.addClass(selected);
                bBox.find("." + ribbons).removeClass(selected);
                bBox.find('.' + ribbons + '[' + attrs.name + '="' + tabName + '"]').addClass(selected);
                bBox.trigger(events[2], {
                    name: tabName,
                    index: a.parent().index(),
                    props: this.tabProps[tabName]
                });
                this.expand()
            }, this));
            bBox.delegate('input:checkbox[name]:not([disabled]), input:radio[name]:not([disabled])', "change", $.proxy(function(e) {
                var a = $(e.currentTarget),
                    command = a.attr("name"),
                    toolType = a.parents("[data-tool-type]").attr("data-tool-type"),
                    eventData = {
                        command: command,
                        value: a.val() || null,
                        active: a.get(0).checked
                    };
                if (this._enabled) {
                    if (["checkbox", "radios"].indexOf(toolType) > -1) {
                        eventData.props = "checkbox" === toolType ? this.props[command] || null : this.props[command][a.parent().parent().index()]
                    } else {
                        if (a.parents("[data-tool]").attr("data-tool") === "toggle-dropdown" || a.parents("[data-tool]").attr("data-tool") === "exclusive-dropdown") {
                            if ("radio" === a.attr("type")) {
                                eventData.props = this.props[command][a.parent().parent().index()]
                            } else {
                                var b = a.parent().parent().index(),
                                    toolName = a.parents("[data-tool-name]").attr("data-tool-name");
                                eventData.props = this.props[toolName][b]
                            }
                        }
                    }
                    bBox.trigger(events[0], eventData)
                }
            }, this));
            bBox.delegate('a[name]:not([data-type="menu"]):not([disabled]):not([data-type*="-dropdown"])', "click", $.proxy(function(e) {
                var a = $(e.currentTarget);
                e.preventDefault();
                if (a.is("[disabled]") || (a.parents("ul").is("." + c.buttonsExclusive) && a.is("." + active))) {
                    return
                }
                if (this._enabled) {
                    var b = a.attr("name"),
                        index, toolType = a.parents('[data-tool]').attr("data-tool"),
                        eventData = {
                            command: b,
                            value: a.attr(attrs.value) || null,
                            active: !a.hasClass(active),
                            props: this.props[b] || null
                        };
                    if ("dropdown" === toolType) {
                        if (a.parents().is(".acidjs-ui-ribbon-dropdown")) {
                            index = a.parents("li").index()
                        } else {
                            index = a.parents('[data-tool]').find("." + c.selected).parent("li").index()
                        }
                        eventData.props = this.props[b][index]
                    }
                    bBox.trigger(events[0], eventData)
                }
                a.blur()
            }, this));
            bBox.delegate('[data-tool="menu"] > div:first-child a', "click", $.proxy(function(e) {
                var a = $(e.currentTarget);
                this._halt(e);
                a.find(".acidjs-ribbon-arrow").click()
            }, this));
            bBox.delegate('.acidjs-ribbon-generic-dropdown > div:first-child a', "click", $.proxy(function(e) {
                var a = $(e.currentTarget);
                this._halt(e);
                a.find(".acidjs-ribbon-arrow").click()
            }, this));
            bBox.delegate(".acidjs-ribbon-generic-dropdown .acidjs-ribbon-arrow", "click", $.proxy(function(e) {
                this._halt(e);
                var a = $(e.currentTarget),
                    dropDown = a.parents(".acidjs-ribbon-generic-dropdown").find("." + c.ribbonDropdown);
                this._hideAllDropdowns();
                this._openDropdown(dropDown)
            }, this));
            bBox.delegate(c.colorPickerDropdownArrow, "click", $.proxy(function(e) {
                this._halt(e);
                var a = $(e.currentTarget),
                    dropDown = a.parents("." + c.colorPicker).find("." + c.ribbonDropdown);
                this._hideAllDropdowns();
                this._openDropdown(dropDown)
            }, this));
            bBox.delegate(c.dropdownArrow, "click", $.proxy(function(e) {
                this._halt(e);
                var a = $(e.currentTarget),
                    dropDown = a.parents("." + c.dropdown).find("." + c.ribbonDropdown);
                this._hideAllDropdowns();
                this._openDropdown(dropDown)
            }, this));
            bBox.delegate(c.splitButtonArrow, "click", $.proxy(function(e) {
                this._halt(e);
                var a = $(e.target),
                    dropDown = a.parents("." + c.splitButton).find("." + c.ribbonDropdown);
                this._hideAllDropdowns();
                this._openDropdown(dropDown)
            }, this));
            bBox.delegate("." + c.buttonsExclusive + " a", "click", $.proxy(function(e) {
                var a = $(e.currentTarget),
                    group = a.parents("." + c.buttonsExclusive);
                group.find("." + active).removeClass(active);
                a.addClass(active)
            }, this));
            bBox.delegate("." + c.toggleButtons + " a", "click", $.proxy(function(e) {
                var a = $(e.currentTarget);
                a.toggleClass(active)
            }, this));
            bBox.on(events[1], $.proxy(function(e, a) {
                e = e || {};
                a = a || {};
                var b = this.config.defaultSelectedTab ? this.config.defaultSelectedTab : 0;
                bBox.find("." + c.tabButtons + " li:eq(" + b + ") a").trigger("click");
                this._setFontFamilies();
                this._setFontSizes();
                _setTrialLimitations.call(this, this)
            }, this));
            this.ready = true;
            bBox.trigger(events[1], {
                ready: this.ready,
                config: this.config
            })
        },
        _setFontSizes: function() {
            var b = this.ATTRS,
                nodes = this.getBoundingBox().find('div[data-tool="dropdown"][data-tool-name="font-size"] .acidjs-ui-ribbon-dropdown').find("a");
            nodes.each(function() {
                var a = $(this),
                    fontSize = a.attr(b.value);
                a.find("span").css({
                    fontSize: fontSize + "px",
                    lineHeight: fontSize + "px"
                })
            })
        },
        _setFontFamilies: function() {
            var b = this.ATTRS,
                nodes = this.getBoundingBox().find('div[data-tool="dropdown"][data-tool-name="font-family"] .acidjs-ui-ribbon-dropdown').find("a");
            nodes.each(function() {
                var a = $(this),
                    fontFamily = a.attr(b.value),
                    fontFamilyArray = [];
                fontFamily = fontFamily.split(",");
                for (var i = 0; i < fontFamily.length; i++) {
                    fontFamilyArray.push("'" + fontFamily[i].trim() + "'")
                }
                fontFamilyArray = fontFamilyArray.join(",");
                a.css({
                    fontFamily: fontFamilyArray
                })
            })
        },
        _openDropdown: function(a) {
            var b = this.CSS_CLASSES,
                cssSelected = b.selected,
                selectedItem = a.find("." + cssSelected);
            a.addClass(b.open);
            if (!selectedItem.is(":visible")) {
                selectedItem.removeClass(cssSelected);
                selectedItem = selectedItem.parent().next().find("a");
                selectedItem.addClass(cssSelected)
            }
            selectedItem.focus()
        },
        _hideAllDropdowns: function() {
            var a = this.CSS_CLASSES.open,
                dropdowns = B.find("." + a);
            dropdowns.removeClass(a);
            dropdowns.find("a").blur()
        },
        _setTemplate: function(a, b) {
            if (a && b) {
                this.TEMPLATES[a] = b.split("\n")
            }
        },
        _createNgDirectives: function(c) {
            var d = [];
            $.each(c, function(a, b) {
                a = a.indexOf("data-") > -1 ? a : "ng-" + a;
                d.push(a + '="' + b + '"')
            });
            return d.join(" ")
        },
        _moveSelection: function(a) {
            var b = this.CSS_CLASSES,
                activeDropdown = $("." + b.open),
                focusedItem = activeDropdown.find("a:focus"),
                focusedItemParent = focusedItem.parent();
            if (!activeDropdown.length) {
                return
            }
            switch (a) {
                case 38:
                    focusedItemParent.prev().find("a").focus();
                    break;
                case 40:
                    focusedItemParent.next().find("a").focus();
                    break
            }
        }
    };
    W.AcidJs.Ribbon = Ribbon;
    var q = W.AcidJs.Ribbon.prototype;
    $(D).bind("click", function(event) {
        q._hideAllDropdowns()
    });
    $(D).keyup(function(e) {
        var a = e.keyCode;
        switch (a) {
            case 27:
                q._hideAllDropdowns();
                break;
            case 38:
            case 40:
                q._moveSelection(a);
                break
        }
    })
})();