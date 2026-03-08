#import "metadata.typ"

#set page(
	paper: "a4",
	margin: 2.5pt,
	fill: white
)

#v(4cm)
#align(center)[
	#image(
		"assets/images/itpln_logo.png",
		width: 4cm
	)
	#v(4pt)
	#text(#proj_title, weight: "bold", size: 16pt, font: "Times New Roman")\

	#v(2.5pt)
	#text(proj_desc, weight: "bold", size: 14pt, font: "Times New Roman")\

	#v(4pt)
	#text("Disusun oleh:", size: 12pt, font: "Times New Roman")\

	#v(2.5pt)
	#grid(
		columns: (27%, 46%, 27%),
		rows: 0.75cm,
		for i in teams {
			[[
				#rect(
					fill: yellow,
					width: 100%
				)
			], [
				#align(left)[
					#text(i, size: 12pt, font: "Times New Roman")\
				]
			], [
				#rect(
					fill: yellow,
					width: 100%
				)
			]]
		},
		[
			#rect(
				fill: yellow,
				width: 100%
			)
		], [
			#align(left)[
				#text("Rafi Indra Pramudhito Zuhayr", size: 12pt, font: "Times New Roman")\
			]
		], [
			#rect(
				fill: yellow,
				width: 100%
			)
		],
		[
			#rect(
				fill: yellow,
				width: 100%
			)
		], [
			#align(left)[
				#text("Rafi Indra Pramudhito Zuhayr", size: 12pt, font: "Times New Roman")\
			]
		], [
			#rect(
				fill: yellow,
				width: 100%
			)
		],
		[
			#rect(
				fill: yellow,
				width: 100%
			)
		], [
			#align(left)[
				#text("Rafi Indra Pramudhito Zuhayr", size: 12pt, font: "Times New Roman")\
			]
		], [
			#rect(
				fill: yellow,
				width: 100%
			)
		]
	)
]