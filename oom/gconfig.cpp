//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: gconfig.cpp,v 1.15.2.13 2009/12/01 03:52:40 terminator356 Exp $
//
//  (C) Copyright 1999-2003 Werner Schweer (ws@seh.de)
//=========================================================

#include <QDir>
#include "gconfig.h"

GlobalConfigValues config = {
	190, // globalAlphaBlend
	{
		QColor(0xff, 0xff, 0xff), // palette
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff),
		QColor(0xff, 0xff, 0xff)
	},
	{
		QColor(0,0,0),       // Splash Menu Part Color //{{{
		QColor(145,255,250), // Saltwater Splash
		QColor(225,145,248), // Bubblegum Splash
		QColor(158,255,145), // Key Lime Splash
		QColor(186,145,255), // Violet Splash
		QColor(255,196,145), // Orange Splash
		QColor(145,182,255), // Freshwater Splash
		QColor(254,255,145), // Lemon Splash
		QColor(0,0,0),       // Frosting Menu
		QColor(231,206,213), // Cranberry Frosting
		QColor(220,207,232), // Grape Frosting
		QColor(207,207,232), // Blueberry Frosting
		QColor(205,230,225), // Matcha Frosting
		QColor(206,231,209), // Green Apple Frosting
		QColor(230,216,205), // Mochaccino Frosting
		QColor(0,0,0),       // Day Off Menu
		QColor(228,221,202), // Day Off: Sea & Sand 
		QColor(255,220,252), // Day Off: Table Cloth
		QColor(200,211,240), // Day Off: His Golf Shirt 
		QColor(255,222,252), // Day Off: Her Golf Shirt
		QColor(215,255,191), // Day Off: Baby's Golf Shirt
		QColor(243,232,233), // Day Off: Book & Blanket
		QColor(230,221,196), // Day Off: Almond Roca
		QColor(223,244,116), // Day Off: Kiwi Cabana
		QColor(239,239,239), // Day Off: Cherry on the Side
		QColor(255,208,200), // Day Off: Redwood Grill
		QColor(195,193,169), // Day Off: Appetizer
		QColor(0,0,0),       // Drinks Menu
		QColor(249,241,237), // Drinks: Raspberry Milkshake
		QColor(250,243,240), // Drinks: Marshmallow Hot Chocolate
		QColor(192,255,185), // Drinks: Irish Hot Chocolate
		QColor(242,232,229), // Drinks: Mochaccino
		QColor(242,232,229), // Drinks: Cream Soda Float
		QColor(219,189,135), // Drinks: Pumpkin Spice Latte
		QColor(251,226,246), // Drinks: Strawberry Apple Smoothie
		QColor(224,188,221), // Drinks: Very Berry Smoothie
		QColor(253,249,123), // Drinks: Lemonade
		QColor(229,255,210), // Drinks: Good Morning Glass
		QColor(211,225,210), // Drinks: Wintermint Steamer
		QColor(0,0,0),       // Designer Menu
		QColor(255,255,255), // Designer: Tux
		QColor(192,192,192), // Designer: Black Tie
		QColor(188,187,186), // Designer: Dad's Suit
		QColor(208,202,193), // Designer: Khaki
		QColor(201,201,201), // Designer: Midnight Stars
		QColor(255,255,255), // Designer: Friday Valet
		QColor(228,231,205), // Designer: Veranda Olivia
		QColor(255,255,255), // Designer: Gentlemen
		QColor(253,197,249), // Designer: Wendy's Dress
		QColor(0,0,0),       // Glow Sticks Menu
		QColor(51,255,248),  // Glow Sticks: Electric Teal
		QColor(255,51,214),  // Glow Sticks: Paint Spill Pink
		QColor(51,255,75),   // Glow Sticks: Go
		QColor(186,51,255),  // Glow Sticks: Purple City
		QColor(255,167,51),  // Glow Sticks: Orange Sherbet
		QColor(51,167,255),  // Glow Sticks: Blue Blast
		QColor(248,255,51),  // Glow Sticks: Bumble Bee
		QColor(0,0,0),       // Spring Menu
		QColor(191,255,251), // Spring: Salt Water Splash
		QColor(255,191,232), // Spring: Rosewater Splash
		QColor(196,255,191), // Spring: Mint Splash
		QColor(229,191,255), // Spring: Lilac Splash
		QColor(255,224,191), // Spring: Orange Splash
		QColor(191,211,255), // Spring: Freshwater Splash
		QColor(255,253,191), // Spring: Lemon Splash
		QColor(0,0,0),       // menu:Summer
		QColor(255,251,131), // Sun Umbrella
		QColor(143,184,255), // Ice Water
		QColor(49,253,242),  // The Pool
		QColor(49,253,242),  // Night Pool
		QColor(253,173,255), // Picnic
		QColor(255,184,92),  // Track Pants
		QColor(159,255,152), // Overpriced Ice Cream 
		QColor(159,255,152), // His Towel
		QColor(255,179,253), // Wendy's Towel
		QColor(0,0,0),       // menu:Autumn
		QColor(251,206,0),   // October
		QColor(253,105,0),   // 70s Hoodie
		QColor(253,207,0),   // Leaf Pile
		QColor(253,197,95),  // Wheat Harvest
		QColor(228,199,132), // Canadian Maple
		QColor(255,174,0),   // Changing Tree
		QColor(249,175,130), // Peach Pie
		QColor(246,239,220), // Birch and Bark
		QColor(229,200,133), // Last Grass
		QColor(219,189,135), // Pumpkin Spice
		QColor(0,0,0),       // menu:Winter
		QColor(255,251,250), // Sled
		QColor(168,209,255), // Frost
		QColor(255,251,250), // Evergreen
		QColor(254,250,249), // Big Boots
		QColor(235,244,255), // Icicle
		QColor(255,214,221), // Rosy Cheeks
		QColor(238,232,228), // Snowman's Smile
		QColor(232,226,225), // Winter Tires
		QColor(171,171,171), // Short Days
		QColor(0,0,0),       // menu:Naturals
		QColor(229,196,177), // Breathe
		QColor(198,181,169), // Bridge and Breeze
		QColor(217,194,188), // Serene Clay
		QColor(219,202,192), // Argan Oil
		QColor(232,203,189), // Stoney Stream
		QColor(219,182,171), // Hazelnut Candle
		QColor(204,163,163), // R and R
		QColor(232,191,155), // Shoeless Shore
		QColor(232,205,165), // Jamaican Rainforest
		QColor(0,0,0),       // OOM Menu
		QColor(175,235,255), // ccherrett
		QColor(255,195,0),   // Jus'maican a DAW
		QColor(179,203,232), // A Stone
		QColor(255,255,255), // The Driving Force
		QColor(255,198,250)  // Wendy Cherrett//}}}
	},
	{
		QColor(0,0,0),    // Splash Menu Wave Colors //{{{
		QColor(63,63,63), // Saltwater Splash
		QColor(63,63,63), // Bubblegum Splash
		QColor(63,63,63), // Key Lime Splash
		QColor(63,63,63), // Violet Splash
		QColor(63,63,63), // Orange Splash
		QColor(63,63,63), // Freshwater Splash
		QColor(63,63,63), // Lemon Splash
		QColor(0,0,0),     // Frosting Menu
		QColor(87,48,59),  // Cranberry Frosting 
		QColor(68,48,87),  // Grape Frosting
		QColor(48,47,86),  // Blueberry Frosting
		QColor(47,86,78),  // Matcha Frosting
		QColor(47,86,53),  // Green Apple Frosting
		QColor(87,65,48),  // Mochaccino Frosting
		QColor(0,0,0),     // Day Off Menu
		QColor(67,81,88),  // Day Off: Sea & Sand 
		QColor(110,82,108),// Day Off: Table Cloth
		QColor(16,28,64),  // Day Off: His Golf Shirt 
		QColor(16,28,64),  // Day Off: Her Golf Shirt
		QColor(16,28,64),  // Day Off: Baby's Golf Shirt
		QColor(90,99,79),  // Day Off: Book & Blanket
		QColor(52,41,39),  // Day Off: Almond Roca
		QColor(32,36,16),  // Day Off: Kiwi Cabana
		QColor(102,30,36), // Day Off: Cherry on the Side
		QColor(101,29,35), // Day Off: Redwood Grill
		QColor(71,70,13),  // Day Off: Appetizer
		QColor(0,0,0),     // Drinks Menu
		QColor(154,43,82), // Drinks: Raspberry Milkshake
		QColor(65,32,17),  // Drinks: Marshmallow Hot Chocolate
		QColor(65,32,17),  // Drinks: Irish Hot Chocolate
		QColor(109,81,68), // Drinks: Mochaccino
		QColor(221,28,107),// Drinks: Cream Soda Float
		QColor(130,65,27), // Drinks: Pumpkin Spice Latte
		QColor(48,124,30), // Drinks: Strawberry Apple Smoothie
		QColor(81,83,112), // Drinks: Very Berry Smoothie
		QColor(51,51,51),  // Drinks: Lemonade
		QColor(133,36,223),// Drinks: Good Morning Glass
		QColor(36,46,35),  // Drinks: Wintermint Steamer
		QColor(0,0,0),     // Designer Menu
		QColor(24,24,24),  // Designer: Tux
		QColor(0,0,0),     // Designer: Black Tie
		QColor(42,16,15),  // Designer: Dad's Suit
		QColor(0,0,0),     // Designer: Khaki
		QColor(0,6,55),    // Designer: Midnight Stars
		QColor(96,1,0),    // Designer: Friday Valet
		QColor(38,36,0),   // Designer: Veranda Olivia
		QColor(0,0,0),     // Designer: Gentlemen
		QColor(24,24,24),  // Designer: Wendy's Dress
		QColor(0,0,0),     // Glow Sticks Menu
		QColor(0,0,0),     // Glow Sticks: Electric Teal
		QColor(0,0,0),     // Glow Sticks: Paint Spill Pink
		QColor(0,0,0),     // Glow Sticks: Go
		QColor(0,0,0),     // Glow Sticks: Purple City
		QColor(0,0,0),     // Glow Sticks: Orange Sherbet
		QColor(0,0,0),     // Glow Sticks: Blue Blast
		QColor(0,0,0),     // Glow Sticks: Bumble Bee
		QColor(0,0,0),     // Spring Menu
		QColor(64,64,64),  // Spring: Salt Water Splash
		QColor(64,64,64),  // Spring: Rosewater Splash
		QColor(64,64,64),  // Spring: Mint Splash
		QColor(64,64,64),  // Spring: Lilac Splash
		QColor(64,64,64),  // Spring: Orange Splash
		QColor(64,64,64),  // Spring: Freshwater Splash
		QColor(64,64,64),  // Spring: Lemon Splash
		QColor(0,0,0),     // menu:Summer
		QColor(0,19,83),   // Sun Umbrella
		QColor(0,19,83),   // Ice Water
		QColor(0,127,119), // The Pool
		QColor(0,0,0),     // Night Pool
		QColor(63,0,106),  // Picnic
		QColor(0,19,83),   // Track Pants
		QColor(63,0,106),  // Overpriced Ice Cream 
		QColor(0,19,83),   // His Towel
		QColor(0,19,83),   // Wendy's Towel
		QColor(0,0,0),     // menu:Autumn
		QColor(154,20,0),  // October
		QColor(42,18,0),   // 70s Hoodie
		QColor(97,51,7),   // Leaf Pile
		QColor(45,28,0),   // Wheat Harvest
		QColor(103,38,3),  // Canadian Maple
		QColor(24,61,22),  // Changing Tree
		QColor(144,46,0),  // Peach Pie
		QColor(35,28,23),  // Birch and Bark
		QColor(24,61,22),  // Last Grass
		QColor(130,65,27), // Pumpkin Spice
		QColor(0,0,0),     // menu:Winter
		QColor(119,0,0),   // Sled
		QColor(15,15,15),  // Frost
		QColor(4,43,0),    // Evergreen
		QColor(46,38,30),  // Big Boots
		QColor(0,38,82),   // Icicle
		QColor(59,53,54),  // Rosy Cheeks
		QColor(49,49,49),  // Snowman's Smile
		QColor(0,0,0),     // Winter Tires
		QColor(64,64,64),  // Short Days
		QColor(0,0,0),     // menu:Naturals
		QColor(53,23,7),   // Breathe
		QColor(20,10,0),   // Bridge and Breeze
		QColor(34,32,29),  // Serene Clay
		QColor(80,62,53),  // Argan Oil
		QColor(34,47,63),  // Stoney Stream
		QColor(0,0,0),     // Hazelnut Candle
		QColor(59,45,42),  // R and R
		QColor(87,45,26),  // Shoeless Shore
		QColor(40,54,37),  // Jamaican Rainforest
		QColor(0,0,0),     // OOM Menu
		QColor(1,53,70),   // ccherrett
		QColor(24,61,22),  // Jus'maican a DAW
		QColor(21,37,56),  // A Stone
		QColor(155,0,0),   // The Driving Force
		QColor(77,77,77)   // Wendy Cherrett//}}}
	},
	{
		QColor(0,0,0),       // Splash Menu Part Color Automation//{{{
		QColor(58,174,170),  // Saltwater Splash
		QColor(174,58,167),  // Bubblegum Splash
		QColor(72,174,58),   // Key Lime Splash
		QColor(102,58,174),  // Violet Splash
		QColor(174,113,58),  // Orange Splash
		QColor(58,96,174),   // Freshwater Splash
		QColor(172,174,58),  // Lemon Splash
		QColor(0,0,0),       // Frosting Menu
		QColor(100,94,95),   // Cranberry Frosting
		QColor(97,94,100),   // Grape Frosting
		QColor(94,94,100),   // Blueberry Frosting
		QColor(94,100,99),   // Matcha Frosting
		QColor(94,100,95),   // Green Apple Frosting
		QColor(100,96,94),   // Mochaccino Frosting
		QColor(0,0,0),       // Day Off Menu
		QColor(100,99,94),   // Day Off: Sea & Sand 
		QColor(110,98,109),  // Day Off: Table Cloth
		QColor(94,96,104),   // Day Off: His Golf Shirt 
		QColor(110,100,109), // Day Off: Her Golf Shirt
		QColor(97,108,90),   // Day Off: Baby's Golf Shirt
		QColor(107,103,104), // Day Off: Book & Blanket
		QColor(99,97,91),    // Day Off: Almond Roca
		QColor(93,98,72),    // Day Off: Kiwi Cabana
		QColor(105,105,105), // Day Off: Cherry on the Side
		QColor(109,95,93),   // Day Off: Redwood Grill
		QColor(88,87,82),    // Day Off: Appetizer
		QColor(0,0,0),       // Drinks Menu
		QColor(108,106,104), // Drinks: Raspberry Milkshake
		QColor(106,103,102), // Drinks: Marshmallow Hot Chocolate
		QColor(93,109,91),   // Drinks: Irish Hot Chocolate
		QColor(106,103,102), // Drinks: Mochaccino
		QColor(106,103,102), // Drinks: Cream Soda Float
		QColor(91,85,75),    // Drinks: Pumpkin Spice Latte
		QColor(110,100,108), // Drinks: Strawberry Apple Smoothie
		QColor(98,90,97),    // Drinks: Very Berry Smoothie
		QColor(101,100,73),  // Drinks: Lemonade
		QColor(102,110,96),  // Drinks: Good Morning Glass
		QColor(95,99,95),    // Drinks: Wintermint Steamer
		QColor(0,0,0),       // Designer Menu
		QColor(110,110,110), // Designer: Tux
		QColor(88,88,88),    // Designer: Black Tie
		QColor(86,86,86),    // Designer: Dad's Suit
		QColor(92,91,90),    // Designer: Khaki
		QColor(91,91,91),    // Designer: Midnight Stars
		QColor(90,90,90),    // Designer: Friday Valet
		QColor(101,101,85),  // Designer: Veranda Olivia
		QColor(111,111,111), // Designer: Gentlemen
		QColor(109,91,107),  // Designer: Wendy's Dress
		QColor(0,0,0),       // Glow Sticks Menu
		QColor(68,110,108),  // Glow Sticks:  Electric Teal
		QColor(110,68,101),  // Glow Sticks:  Paint Spill Pink
		QColor(68,110,73),   // Glow Sticks:  Go
		QColor(96,68,110),   // Glow Sticks:  Purple City
		QColor(110,92,68),   // Glow Sticks:  Orange Sherbet
		QColor(68,92,110),   // Glow Sticks:  Blue Blast
		QColor(108,110,68),  // Glow Sticks:  Bumble Bee
		QColor(0,0,0),       // Spring Menu
		QColor(101,159,155), // Spring: Salt Water Splash
		QColor(159,101,138), // Spring: Rosewater Splash
		QColor(105,159,101), // Spring: Mint Splash
		QColor(135,101,159), // Spring: Lilac Splash
		QColor(159,132,101), // Spring: Orange Splash
		QColor(101,119,159), // Spring: Freshwater Splash
		QColor(159,157,101), // Spring: Lemon Splash
		QColor(0,0,0),       // menu:Summer
		QColor(136,134,86),  // Sun Umbrella
		QColor(86,109,143),  // Ice Water
		QColor(67,107,105),  // The Pool
		QColor(67,107,105),  // Night Pool
		QColor(151,95,153),  // Picnic
		QColor(123,103,77),  // Track Pants
		QColor(95,147,91),   // Overpriced Ice Cream 
		QColor(94,143,91),   // His Towel
		QColor(155,97,153),  // Wendy's Towel
		QColor(0,0,0),       // menu:Autumn
		QColor(110,97,36),   // October
		QColor(112,68,36),   // 70s Hoodie
		QColor(110,97,36),   // Leaf Pile
		QColor(134,113,58),  // Wheat Harvest
		QColor(137,118,71),  // Canadian Maple
		QColor(112,88,36),   // Changing Tree
		QColor(160,98,60),   // Peach Pie
		QColor(170,151,102), // Birch and Bark
		QColor(139,119,71),  // Last Grass
		QColor(91,85,75),    // Pumpkin Spice
		QColor(0,0,0),       // menu:Winter
		QColor(168,135,126), // Sled
		QColor(94,121,152),  // Frost
		QColor(168,135,126), // Evergreen
		QColor(168,135,126), // Big Boots
		QColor(116,141,166), // Icicle
		QColor(164,110,119), // Rosy Cheeks
		QColor(142,135,130), // Snowman's Smile
		QColor(136,132,130), // Winter Tires
		QColor(100,100,100), // Short Days
		QColor(0,0,0),       // menu:Naturals
		QColor(146,111,90),  // Breathe
		QColor(117,105,97),  // Bridge and Breeze
		QColor(133,109,103), // Serene Clay
		QColor(136,116,104), // Argan Oil
		QColor(151,113,95),  // Stoney Stream
		QColor(134,102,92),  // Hazelnut Candle
		QColor(122,92,92),   // R and R
		QColor(147,111,79),  // Shoeless Shore
		QColor(149,122,83),  // Jamaican Rainforest
		QColor(0,0,0),    	 // OOM Menu Colors
		QColor(63,156,187),  // ccherrett
		QColor(112,95,36),   // Jus'maican a DAW
		QColor(88,117,152),  // A Stone
		QColor(104,104,104), // The Driving Force
		QColor(192,72,182)   // Wendy Cherrett//}}}
	},
	{
		QColor(0,0,0),    // Splash Menu Automation Wave Colors//{{{
		QColor(63,63,63), // Saltwater Splash
		QColor(63,63,63), // Bubblegum Splash
		QColor(63,63,63), // Key Lime Splash
		QColor(63,63,63), // Violet Splash
		QColor(63,63,63), // Orange Splash
		QColor(63,63,63), // Freshwater Splash
		QColor(63,63,63), // Lemon Splash
		QColor(0,0,0),    // Frosting Menu
		QColor(44,34,37), // Cranberry Frosting 
		QColor(39,34,44), // Grape Frosting
		QColor(33,33,43), // Blueberry Frosting
		QColor(33,43,41), // Matcha Frosting
		QColor(33,43,35), // Green Apple Frosting
		QColor(44,38,34), // Mochaccino Frosting
		QColor(0,0,0),    // Day Off Menu
		QColor(43,47,49), // Day Off: Sea & Sand 
		QColor(60,52,59), // Day Off: Table Cloth
		QColor(17,20,29), // Day Off: His Golf Shirt 
		QColor(17,20,29), // Day Off: Her Golf Shirt
		QColor(17,20,29), // Day Off: Baby's Golf Shirt
		QColor(52,55,49), // Day Off: Book & Blanket
		QColor(29,26,25), // Day Off: Almond Roca
		QColor(17,18,12), // Day Off: Kiwi Cabana
		QColor(48,30,31), // Day Off: Cherry on the Side
		QColor(47,29,30), // Day Off: Redwood Grill
		QColor(31,31,17), // Day Off: Appetizer
		QColor(0,0,0),    // Drinks Menu
		QColor(73,43,54), // Drinks: Raspberry Milkshake
		QColor(30,22,18), // Drinks: Marshmallow Hot Chocolate
		QColor(30,22,18), // Drinks: Irish Hot Chocolate
		QColor(57,50,47), // Drinks: Mochaccino
		QColor(100,48,69),// Drinks: Cream Soda Float
		QColor(60,43,32), // Drinks: Pumpkin Spice Latte
		QColor(37,57,33), // Drinks: Strawberry Apple Smoothie
		QColor(53,54,61), // Drinks: Very Berry Smoothie
		QColor(30,30,30), // Drinks: Lemonade
		QColor(77,51,101),// Drinks: Good Morning Glass
		QColor(23,25,23), // Drinks: Wintermint Steamer
		QColor(0,0,0),    // Designer Menu
		QColor(13,13,13), // Designer: Tux
		QColor(0,0,0),    // Designer: Black Tie
		QColor(21,14,13), // Designer: Dad's Suit
		QColor(0,0,0),    // Designer: Khaki
		QColor(8,9,22),   // Designer: Midnight Stars
		QColor(40,14,14), // Designer: Friday Valet
		QColor(15,14,5),  // Designer: Veranda Olivia
		QColor(0,0,0),    // Designer: Gentlemen
		QColor(13,13,13), // Designer: Wendy's Dress
		QColor(0,0,0),    // Glow Sticks Menu
		QColor(0,0,0),    // Glow Sticks: Electric Teal
		QColor(0,0,0),    // Glow Sticks: Paint Spill Pink
		QColor(0,0,0),    // Glow Sticks: Go
		QColor(0,0,0),    // Glow Sticks: Purple City
		QColor(0,0,0),    // Glow Sticks: Orange Sherbet
		QColor(0,0,0),    // Glow Sticks: Blue Blast
		QColor(0,0,0),    // Glow Sticks: Bumble Bee
		QColor(0,0,0),    // Spring Menu
		QColor(37,37,37), // Spring: Salt Water Splash
		QColor(37,37,37), // Spring: Rosewater Splash
		QColor(37,37,37), // Spring: Mint Splash
		QColor(37,37,37), // Spring: Lilac Splash
		QColor(37,37,37), // Spring: Orange Splash
		QColor(37,37,37), // Spring: Freshwater Splash
		QColor(37,37,37), // Spring: Lemon Splash
		QColor(0,0,0),    // menu:Summer
		QColor(17,20,29), // Sun Umbrella
		QColor(17,20,29), // Ice Water
		QColor(27,45,44), // The Pool
		QColor(0,0,0),    // Night Pool
		QColor(31,23,37), // Picnic
		QColor(17,20,29), // Track Pants
		QColor(31,23,37), // Overpriced Ice Cream 
		QColor(17,20,29), // His Towel
		QColor(17,20,29), // Wendy's Towel
		QColor(0,0,0),    // menu:Autumn
		QColor(67,28,21), // October
		QColor(17,10,5),  // 70s Hoodie
		QColor(42,29,16), // Leaf Pile
		QColor(19,14,5),  // Wheat Harvest
		QColor(44,26,16), // Canadian Maple
		QColor(18,29,17), // Changing Tree
		QColor(62,34,20), // Peach Pie
		QColor(18,16,14), // Birch and Bark
		QColor(18,29,17), // Last Grass
		QColor(60,43,32), // Pumpkin Spice
		QColor(0,0,0),    // menu:Winter
		QColor(42,26,26), // Sled
		QColor(7,7,7),    // Frost
		QColor(9,14,8),   // Evergreen
		QColor(22,21,20), // Big Boots
		QColor(17,23,29), // Icicle
		QColor(31,31,31), // Rosy Cheeks
		QColor(27,27,27), // Snowman's Smile
		QColor(0,0,0),    // Winter Tires
		QColor(37,37,37), // Short Days
		QColor(0,0,0),    // menu:Naturals
		QColor(22,14,10), // Breathe
		QColor(8,6,4),    // Bridge and Breeze
		QColor(18,17,16), // Serene Clay
		QColor(42,37,34), // Argan Oil
		QColor(23,27,31), // Stoney Stream
		QColor(0,0,0),    // Hazelnut Candle
		QColor(31,28,27), // R and R
		QColor(41,29,23), // Shoeless Shore
		QColor(24,28,24),  // Jamaican Rainforest
		QColor(0,0,0),     // OOM Menu
		QColor(10,25,30),  // ccherrett
		QColor(18,29,17),  // Jus'maican a DAW
		QColor(16,20,26),  // A Stone
		QColor(67,25,25),  // The Driving Force
		QColor(43,43,43)   // Wendy Cherrett 
		//}}}
	},
	{
		QString("menu:Splash"),//{{{ Part Color Names
		QString("Saltwater Splash"),
		QString("Bubblegum Splash"),
		QString("Key Lime Splash"),
		QString("Violet Splash"),
		QString("Orange Splash"),
		QString("Freshwater Splash"),
		QString("Lemon Splash"),
		QString("menu:Frostings"),
		QString("Cranberry Frosting"),
		QString("Grape Frosting"),
		QString("Blueberry Frosting"),
		QString("Matcha Frosting"),
		QString("Green Apple Frosting"),
		QString("Cappuccino Frosting"),
		QString("menu:Day Off"),
		QString("Sea and Sand"),
		QString("Tablecloth"),
		QString("His Golf Shirt"),
		QString("Her Golf Shirt"),
		QString("Baby's Golf Shirt"),
		QString("Book and Blanket"),
		QString("Almond Roca"),
		QString("Kiwi Cabana"),
		QString("Cherry on the Side"),
		QString("Redwood Grill"),
		QString("Appetizer"),
		QString("menu:Drinks"),
		QString("Raspberry Milkshake"),
		QString("Marshmallow Hot Chocolate"),
		QString("Irish Hot Chocolate"),
		QString("Mochaccino"),
		QString("Cream Soda Float"),
		QString("Pumpkin Spice Latte"),
		QString("Strawberry Apple Smoothie"),
		QString("Very Berry Smoothie"),
		QString("Lemonade"),
		QString("Good Morning Glass"),
		QString("Wintermint Steamer"),
		QString("menu:Designer"),
		QString("Tux"),
		QString("Black Tie"),
		QString("Dad's Suit"),
		QString("Khaki"),
		QString("Midnight Stars"),
		QString("Friday Valet"),
		QString("Veranda Olivia"),
		QString("Gentlemen"),
		QString("Wendy's Dress"),
		QString("menu:Glow Sticks"),
		QString("Electric Teal"),
		QString("Paint Spill Pink"),
		QString("Go"),
		QString("Purple City"),
		QString("Orange Sherbet"),
		QString("Blue Blast"),
		QString("Bumble Bee"),
		QString("menu:Spring"),
		QString("Botanical Spray"),
		QString("Cherry Blossom"),
		QString("Spring Mint"),
		QString("Lilac Song"),
		QString("Peach Mousse"),
		QString("Smell of Rain"),
		QString("Duckling"),
		QString("menu:Summer"),
		QString("Sun Umbrella"),
		QString("Ice Water"),
		QString("The Pool"),
		QString("Night Pool"),
		QString("Picnic"),
		QString("Track Pants"),
		QString("Overpriced Ice Cream"),
		QString("His Towel"),
		QString("Wendy's Towel"),
		QString("menu:Autumn"),
		QString("October"),
		QString("70s Hoodie"),
		QString("Leaf Pile"),
		QString("Wheat Harvest"),
		QString("Canadian Maple"),
		QString("Changing Tree"),
		QString("Peach Pie"),
		QString("Birch and Bark"),
		QString("Last Grass"),
		QString("Pumpkin Spice"),
		QString("menu:Winter"),
		QString("Sled"),
		QString("Frost"),
		QString("Evergreen"),
		QString("Big Boots"),
		QString("Icicle"),
		QString("Rosy Cheeks"),
		QString("Snowman's Smile"),
		QString("Winter Tires"),
		QString("Short Days"), 
		QString("menu:Naturals"),
		QString("Breathe"), 
		QString("Bridge and Breeze"), 
		QString("Serene Clay"), 
		QString("Argan Oil"), 
		QString("Stoney Stream"), 
		QString("Hazelnut Candle"), 
		QString("R and R"), 
		QString("Shoeless Shore"), 
		QString("Jamaican Rainforest"), 
		QString("menu:OOM Colors"),
		QString("ccherrett"),
		QString("Jus'maican a DAW"),
		QString("A Stone"),
		QString("The Driving Force"),
		QString("Wendy Cherrett")//}}}
	}
,
	QColor(16, 24, 25), // transportHandleColor;
	QColor(66, 202, 230), // bigTimeForegroundColor;
	QColor(16, 24, 25), // bigTimeBackgroundColor;
	QColor(23, 23, 23), // waveEditBackgroundColor;
	{
		QFont(QString("arial"), 10, QFont::Normal),
		QFont(QString("arial"), 8, QFont::Normal),
		QFont(QString("arial"), 10, QFont::Normal),
		QFont(QString("arial"), 10, QFont::Bold),
		QFont(QString("arial"), 8, QFont::Bold), // timescale numbers
		QFont(QString("Lucidatypewriter"), 14, QFont::Bold),
		QFont(QString("arial"), 8, QFont::Bold, true) // Mixer strip labels. Looks and fits better with bold + italic than bold alone,
		//  at the price of only few more pixels than Normal mode.
	}
,
	QColor(30, 30, 30), // trackBg;
	QColor(18, 25, 28), // selected track Bg;
	QColor(71, 202, 225), // selected track Fg;

	QColor(0, 160, 255), // midiTrackLabelBg;   // Med blue
	QColor(0, 160, 255), // drumTrackLabelBg;   // Med blue
	Qt::magenta, // waveTrackLabelBg;
	Qt::green, // outputTrackLabelBg;
	Qt::red, // inputTrackLabelBg;
	Qt::yellow, // groupTrackLabelBg;
	QColor(120, 255, 255), // auxTrackLabelBg;    // Light blue
	QColor(255, 130, 0), // synthTrackLabelBg;  // Med orange

	QColor(55, 45, 61), // midiTrackBg;
	QColor(77, 65, 83), // drumTrackBg;
	QColor(40, 62, 68), // waveTrackBg;
	QColor(133, 36, 36), // outputTrackBg;
	QColor(75, 112, 75), // inputTrackBg;
	QColor(58, 64, 65), // groupTrackBg;
	QColor(83, 87, 137), // auxTrackBg;
	QColor(125, 73, 32), // synthTrackBg;

	QColor(63,63,63), // part canvas bg
	QColor(255, 170, 0), // ctrlGraphFg;    Medium orange
	QColor(0, 0, 0), // mixerBg;

	384, // division;
	1024, // rtcTicks
	-60, // int minMeter;
	-60.0, // double minSlider;
	false, // use Jack freewheel
	20, // int guiRefresh;
	QString(""), // userInstrumentsDir
	//QString(""),                  // helpBrowser; // Obsolete
	true, // extendedMidi
	384, // division for smf export
	QString(""), // copyright string for smf export
	1, // smf export file format
	false, // midi export file 2 byte timesigs instead of 4
	true, // optimize midi export file note offs
	true, // Split imported tracks into multiple parts.
	1, // startMode
	QString(""), // start song path
	384, // gui division
	QRect(0, 0, 400, 300), // GeometryMain;
	QRect(0, 0, 200, 100), // GeometryTransport;
	QRect(0, 0, 600, 200), // GeometryBigTime;
	QRect(0, 0, 400, 300), // GeometryPerformer;
	QRect(0, 0, 400, 300), // GeometryDrumedit;
	//QRect(0, 0, 300, 500),        // GeometryMixer;  // Obsolete
	{
		QString("Mixer A"),
		QRect(0, 0, 300, 500), // Mixer1
		true, true, true, true,
		true, true, true, true
	},
	{
		QString("Mixer B"),
		QRect(200, 200, 300, 500), // Mixer2
		true, true, true, true,
		true, true, true, true
	},
	false, // TransportVisible;
	false, // BigTimeVisible;
	false, // mixer1Visible;
	false, // mixer2Visible;

	false, // markerVisible;
	true, // showSplashScreen
	1, // canvasShowPartType 1 - names, 2 events
	5, // canvasShowPartEvent
	true, // canvasShowGrid;
	QString(""), // canvasBgPixmap;
	QStringList(), // canvasCustomBgList
	QString(":/style.qss"), // default styleSheetFile
	QString(""), // style
	QString("sweep"), // externalWavEditor
	false, // useOldStyleStopShortCut
	true, // moveArmedCheckBox
	true, // useDenormalBias
	false, // useOutputLimiter
	true, // showDidYouKnow
	false, // vstInPlace  Enable VST in-place processing
	44100, // Dummy audio preferred sample rate
	512, // Dummy audio buffer size
	QString("./"), // projectBaseFolder
	true,	 // projectStoreInFolder
	false,	// useProjectSaveDialog
	8888, //LSCP client port
	QString("localhost"), //LSCP default hostname
	5, //LSCP client retries
	1,  //LSCP client timeout
	true, //LSCP client use bank name as patch group name
	true, //LSCP client auto start
	false, //Reset LS on client start
	false, //Reset LS on song start
	true, //Start LinuxSampler on startup
	QString("linuxsampler"), //LinuxSampler Path
	true, //Load LV2 plugins
	true, //Load LADSPA plugins with search path below
	//TODO: Update this when windows and mac support comes into play
	QString(QString("/usr/local/lib64/ladspa:/usr/lib64/ladspa:/usr/local/lib/ladspa:/usr/lib/ladspa:").append(QDir::homePath()).append(QDir::separator()).append(".ladspa")),
	true, //Load VST plugins with search path below
	QString(QString("/usr/local/lib64/vst:/usr/lib64/vst:/usr/local/lib/vst:/usr/lib/vst:").append(QDir::homePath()).append(QDir::separator()).append(".vst")),
	0, //Default audio raster index
	1, //Default midi raster index
	true //Use auto crossfades
};

