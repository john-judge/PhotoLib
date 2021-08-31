import PySimpleGUI as sg


class Layouts:

    def __init__(self, data):
        self.data = data
        # Do not change plot size! Tkinter causes shudders and odd resize attempts.
        self.plot_size = (637, 495)
        self.house64 = 'data:image/png;base64,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'
        self.timer64 = 'data:image/png;base64,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'
        self.close64 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsSAAALEgHS3X78AAAE30lEQVRIiZ2VXYgdRRqGn6+quvucM/85iRoTNevMBJFEWY0GFQTBC1HBlaz/jMpoFFfXBdmFvdiLvRIEFRHFGBXMjUQhF/6Bol6sSNaIruCNir/R/Dlx5iRzck736e6qby/6JDlx9CIWFN10Ue/7vW+9X7XcDn8bryWPL2vERkNQQPj9Q72K7F3s7Hxb9bZ98L0bj91jt1y23kxNTxIEGUQ/aTYR6WW9cud/Prx01zf7/7FP5EHXHG7Y6bVTpBPLMSegCWKEEMKvkihgjEWDP+FbEjxTa1bjv9l/CsIKF3ypHhUDSFGACCKC956iKKjV6/hfkCjgUNK0TW1oCA3h+EJk8UUBYFCsQaSyRajArUWLnEONcTrT68nTLtZaEKmmMTiUlsREGy9HO0dgcL1y6lgtZrAsEYFexhwxq2buYfru+1mcOo+828UYg4rgUH7OSkY3zbDq1lkaV1yFP9TqEyy18jiBCMF7DjYmOOu+hxifnCSKItZuvp/F6fPJ05TEwE+dHhN33MfpGy4iFAVjf7qF8etvBV9y1IilBApGIMt6TExOM372JKqKqhLFMdOz93Jk6jx+bHVoztzLyj9eiHqP2Gq7O3UlGAuq1RwYDlUwhoChMdSAz3ZxaEeD8T/fBggaAnGtxpqZWdKFBSbOPLMCCQGJItJPdrHw4lOYRgNsBM6dSCDGErIuodtGkhoyPEr68U5svcbI1ZsQY0CV2vAw9ZGRKjEiSBTR/fQjDm9/AddcjqoSul182kYHVDhJauRffUH7wD7ilatxzVOwI6PM7XiJLO2x4rob0CgGVTSEKigidD94j/ltW9Dg0b0/4BfmyQ8ewKUdWLZ6wCIB9SXFXJvQ+hLkc6QeEznHf199jY1rpjh1w0ZUFTGm7z18/tSj2Hffor5shKLdhhJCADMcw7IlKRIkAqkJRIa4LPl6d5c/PPJkBd5vpArcArD+ue101l1Md08bFxuIBUlOyOUggUIAVIl94Kv5wKqtz7L+7r/0bRHEmApcFbwnHhljw6tv0b3kEtK5gDWmj/GbfQAWZbdaztjyPOfP3oN6D8GDCO133uDAvx9CyxKsRX1JMjbBBa+8Rnbl5RSpR35RfXUGfVLnYGFBcTfdwLo77yLkPYy14CLa773JngfuoNy7QOh2WPnw09WVkufUm8s598G/s+eT9wmBJZ1m+sVTFNBc4Wi8vJ3v//kAJk7AOhbf3MGezTfjWwuYCcv8s1s58K+/okWOxDGdjz5g7+YZtKRSoL+igCp5FKVntGk48sTTzDWb1C+4mB833wgETD2CELBjEfNbtyAjo4xdcz27N11L6B5GGoZQhN+26KiSoII9LebnJx9BkggzNIQkyfEdItiRQGvbM7S2bQHJMGN1NO8ds2dQhBORYBCjAFEE1kFSw0QxuAiTJCAGce64vz4gviTkOTJcErIMMRbyDIxg7bHTFnc47clcmpdj43VkeBRJEkytgdTqSL2OiRMkSRDroH9t4EtCUaBZhmYpIUurZ9pFfVnuX+w62xfjeq3D3/6vbifXrT1XkzgWdREmipA4RlwMUYRY21cg/X+lJ5gSbIHGOVovCHmOCSX7DrbMx599icIhVI2cA5c5mC1gbGnITm4oqAOr0PoOXs9g51HAGiITyCDByXDp4KuiaoESmP8/YC0Y5GajmEsAAAAASUVORK5CYII='
        self.psg64 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAMAAADXqc3KAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAMAUExURQAAABlbkiVhkyZikyFjmShjkyhlly1mli5nlylnmS5olyppnC5qmi5rmzBpmDBpmTFqmDFqmTFqmjNrmzFrnDNsmzJsnDJtnTFunjNvnzRsmTRtmzVunTVunjVvnzZwnjlxny5toDFvozJwoTNzpjVwoDRzpDRzqDd4rThyojp0ozl1pTt2pjt3pz11ozt4qDl4qTp6rTl6rzx4qD55qTx5qj97qz17rT58rD98rT9+rz5+sEB3pEF9rkB+r0F+sEF/sT2AtD6AtT6Btk+Aqk2CrEOBs0GBtEKCtEGCtUSBskWBs0SCtEWDtUaEtkWFt0aGt0KFuUCEukOGu0eFuEWHu0eJvEiGuUiHukiIuUiIukmJu0uKvEyKvFCAp1CCqlODrVKCrl2Kr1OEs1KGsFWGsFaIsVaPvFqKsl6NsmKNsWeTuG6YvHadvlySwV6UxF+YxW+bxW6dxnKewHGex3SdwHSfx3egwXGizHmgwHqkxnyjxHio0f/RMf7QMv/TMf/SMv3SNf7UOv7UO//UPP/UPf/UPv/VP//WPP/XPv/VQ//WQv7XQ//WRP/XRf/WR//YRv/YSP/YSf/YSv/ZS//aS//ZTf/aTP7aTf7aTv7bT//cT//bUP/cUP7cUv/cU/7cVf/eVf/fVv/eV/7dWP/eWP/eWf/fWv/fW//fYvzcaf/hW//gXP/gXv/gX//hYP/hYf/hY//iYP/jY//gZf/iZP7iZf/jZv/iZ//lZv/jaf/kav7ka//maf/ma//kbf/lbv7mbP/mbv/mb//id//mcP/ncv/nc//ld//ndv/meP/ocf/ocv/oc//odP/odf/odv/peP/pff/qfY+11ZSzz5G41qC81aW/1P/jgf/qiv/qjv7qoMnZ5szb587d6eDm2+fo1+7v3e/x3vXw1fHx3gAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAJblQd8AAAEAdFJOU////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////wBT9wclAAAACXBIWXMAABcQAAAXEAEYYRHbAAAAGHRFWHRTb2Z0d2FyZQBwYWludC5uZXQgNC4xLjFjKpxLAAABzUlEQVQoU2P4jwNAJZIEuLJA9M2u/iNgAaiEELOgIFPc//+r6puaalvAQmAJdg4pPj4h5oT/2+raWtqaGmAS/PxC/IJAGdbEB7saW5pb20AyQAkFNiFhUSEgkGZNfjizua29u70XJJHr8j+eV0RGVkRMTJb56u2mvt7eSR0gCT2gPsbMGzbi8hJyPDl3OidPnDRlwhagRHbG/zTXe5WSqqqqmpzXb/VMmz5jztSVIDtSWFLvl3Jrampq8ZY8WThj1tx586ZCXFV9t1xRR1tbR6Lw0f6ZC+YvWDAb6tz/xUom+rrGymWPD8xaunjZ0oUgMZBEsYqZqampWsnTY/PWLF+xZhFIHCRRpW5raWFhUPT/3IJ1a9euW/H//5oTYAlDezs7Kwvv//+XbN6wcev6//+3/z8FltDwcrC3N8/7v3rHtu07Nv3/vxVo0CWQhJGPm5ubdf7/TXt279699//JnTA70j38fH19wv//33b00OGj+w6fPXz5KMRVTiH+/gHuFf//7zl+5szZs2fO7YPo+H/FOSIyPMqz5v//g+dAMocvQCX+XwsMjYmNdgSy9p0/d/bgRZAYWOL//4LgoDAwY+++02AaJoEJcEj8/w8A4UqG4COjF7gAAAAASUVORK5CYII='
        self.cpu64 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsSAAALEgHS3X78AAAFi0lEQVRIiZ1WS2wbRRj+/5l92O6u7WycTW03xLEd6uBUjVO1hfbSByLQEwhxoIhbCwcuvApUiEOFhAQVEiAOvK4gqoojSEhtgUNApaJKG5qkdWpa1JfcZL3teu3sZmeGQ9ZVGhIJ8Z/+2Vn93//45psBWMMqlcoXxWLxEACAaZpju3btOkkIoZRSsnv37hOmaY4BABSLxUOVSuXzteJIq32klMqyLBuqqhoAAKqqGpTSpKIoSUQESZK6OnuKohiKohiUUpkxtvivWCvWBABEOp1+sr+//5V169ZtnJub+6FUKh3Rdf3hVqv1l6Zp5Ww2ezASifQ0Go3fhoaGjsZisYdardaM4zjTiEgAQHQC4j0HkQghAAC4oiiJRCKxBQBIs9m8oOt6iRASa7VaVwEAYrFYP+e85TjOpXg8PiyE4LZtn/F93wYAgogghOD3AYS+UFW1q1AovGoYxp4wGxIEgS2EEIQQCQCAcx7gkslCCB8AwLbt07Va7SPP8xqdWPdmIElSxDTNfZZlncrn828MDg6+VavVPvF9fy4Wi/X19fUdWJHMfSaEYJlMZgwRpVqtdtQwjD31ev2HIAgWpJGRkS8VRTEMw9g9OTm5v7u7+9GpqamXq9XqxwAAmzZt+oBzjpzzYC0QIQRDRJpIJLanUqmdw8PDX1mW9ZPv+5bkOM5FVVVTiURia1i24rruDQCAUqn09sDAwCHGGEdEadnwlgOJZT5BRMIYc5rN5iXP8+ax0y9N04qc84Vt27aduHjx4uuEED46Ovo95xxEOH1ExKWEhQh9DPe4JEl0fn7+14mJiecQUWo2m7MAgNQ0zb3d3d3bhoaGjrTb7Wld1x/p6uoa2bBhw4uyLGsAEFBKKSIi51xQSjFcIiICIQRDAhDXdWue502Vy+X3hRALqqr2SoODg2/KsmzE4/GNlNJ1nPOF9evXPxYEAbiue7lWq72rKIphmub+GzdufBeNRg1d14cZYx4hhBJClFQqNRbOQlBKo8lkcms+n48vLi5a0vj4+OOKoiTT6fQzjuNcJYRIQRCALMswOzv7LSEk0tPT85TjOBeCIKi12+1rtm3/ruv6FgDgAMB7e3vHgiAAQgh1HOfquXPnXr958+Zx3/dtshopltp7nyEiUtd1rxuG8URfX99B13Un2+32rKIo3ZzztRgMdOfOnT/mcrkX+vv79zcajVOapm3XNC3HGINoNNpnWdZJz/P+TiQSOzRNK6bT6WcjkUh/q9WaQUTIZrMHEFEjhECz2fzL9/2ZkZGRz0zT3JfNZp+WqtXq+5FIJJXL5V5kjLVDdgDnnMVisYFyufxVSFHgnO9gjDFElIvF4jth34ExxgCAIiIyxtq2bZ+5cuXK5wsLC3NSvV4/BQDCsqw/hBBBLpeTO+WF/KdhC0TIHAoAIggCjogYMnjpEBAi27Z96ezZsy90aCoVCoXXVFVNZbPZ/TMzMy9xzr1ljSdhYLHicN0DCkFYWKFnGMamUqn06fXr17/xPG9e0nV9Y6jnWqiAPCydrTm5laxY+pcCABdCcEqprmnag4qiWNLExMTBZWI3Ho/Hd2Qymb1CCBpm+V8AQJZluHPnzum5ubnx8+fPH+iI3apync/nX04mk9vDXihCiMX/K9drXTjJZDK5FRHJ3bt3/9R1/cH/e+Esb0FnkKK3t3ff5s2bv+7p6Rm7devWsXK5/GGhUDjsOM5kNBp9oFKpfKNp2kC9Xv9xdHT0eCaTed513fPhlYmd4CsBOiDQarVmu7q6KpZl/XLt2rVjQggvHo8PTE9PH242m1PpdPrRy5cvf3L79u2fo9GoyRi7U61W3wsDL5fv1V8VjLFF3/ct3/ctAADP86wgCBq+7zcAABljtud5FgCA7/uWLMvWai8KAIB/ACsf4Gh+DNwbAAAAAElFTkSuQmCC'
        self.camera64 = 'data:image/png;base64,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'
        self.checkmark64 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsSAAALEgHS3X78AAAD2ElEQVRIibWVe4hUVRzHP+feuTOz82gfrqwu5gM0zTQpS1yfoGQPyc3IjOwlYRElPcDICEP6Q6i/IvojCCIpTA0tSlSqJSvbTLZE0dbWxz4nXWdnxrk7M/feOY/+sHyEa7pt57/z45zP53x/B84R/B/jzeNLgeXADDHE4IeBV2Ihpo6LK1rzNqEhAW9oGw18UhelYeWUGHFL0tx5lqPZyBAI3mi9o8YRm16cWTlsVFLQVwjY2+4SSI2W+j8KXj+ybmwytO69xjo7Wyqzr8sldbaE60ksIdBlhTVo+KuHXppY5azftKzeNsbQkurntOuTKQQobYhFbCgPNsGaA5NDWm94ZlYV7fmAX3pcenIlTucDAqlJRENUxcJQLgwiwfMtYcpq4503JMJjq8M0d+XpyBRJnfUpBpJwyKYqFqbCcSCQg0gQyCeq4qHp90yr5Pd0kY6+ImnXJ1CaeDhEdSJCTSJKzLEHLXhu4oQEuWKZ79uzZAoX2hKPhOn+I6DtuEdfLriC4NE9L4CYhzEP8dH84Hz9kT0NBHLqvMlJmo5nyBQDylITj4RwM5rmw70orcEA0AL8Q/DgN8OBr/DltL8q64G1F52+obomwr6US7boE0hNhRPiVIdHx7H+EvA2sJ0tC3/+e8uFS27c/SS+7ElGrGkbnp5EfV0UArmGxt0Lzq/x5YzKWocz/T4FXyGEINvj0XE410QgJ7Fl4dqL4ecS3PVlJYgdllKzx04ZxqolY8h4mkm315JPl+z+nP8Bd++4hZ2LM/hyuokLCr7Eti28TJnOA5ndGLOUnYtLl+u2YMHnJ4BxY2bWsWj2SA72eoBBG4PnBvy2qwvpq81gVjhJp1Q7q9axLIFVMqSaz3ytfLWEpsbLwgFs6pc1o/R9+e7+eK9joSMWvjR4gSLA4FSGKLS7UyirUmRkbJFTG0VI6N17+oR0/bl8d/+A8HMJAG7bPB7BTmGL8TVz64mMiKGNQSuN0hqvq59CS59Kzq2zo8MrcH/s1V6qMIf9y5uvBL8gALj54xpgG5aYH589klB9BdoYjDY0XJ9k9HURPj2aRZ/ycL/tfouDK17+N/ilAoAbP6wAsRGLB8INI7BGJUAYLGEhLAtLCApfnDymc95NtD4eDMC8ZNiXzNKfSdLbt5K8N6o68nNMwoHqKCAwlkVwKI06ln2MtpWtVwMHBnjspHyNQO1Xe7pRbTmUEchCGbk/laKsdl0tfGBB51OKQM0hUD/ppk7kkTTy11NQku/TuUpdi+DKn/7wdyuAHzDcii0Uykwg/ezJoRMAVL9TCWwFjpJdvfpa4AB/Akx4zQw8GDagAAAAAElFTkSuQmCC'
        self.cookbook64 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABQAAAAYCAIAAAB1KUohAAAACXBIWXMAAAsSAAALEgHS3X78AAADMUlEQVQ4jY1UQUgCWxS9782M5hQzle3NTRGEQdBipCKKECoEg4JaJAqCmyBsYauEaNOqjQtXLSKINuFiKMiwIMtF1iLK2kWRZLUoDRxo5s37ixfSt4+/u7xzz51z7r3nwc7Ojt1uBwCEEPwtWKXL5eIvLy+HhoYIIYIgCIJAKa0Do5RyHPfy8nJycnJ1dcUjhJxOZygUOj09zefzFoulDp5SihCKRqPLy8vJZBI4jgOAo6Mjj8cDABjj/6WdTqdDoRAAfJeyFn8MQohhGADAY4xFUSyVSpIkAYBpmgih+soRQmxm2GazbW5u7u7ujoyMKIrCmP+ePMdxv9nhSqXi8/lmZmb29vay2Syrs1gs8EM/QogQQgipBWOMOzs7397eWlpabDYbAMiyHAwGu7u7mQTWzu/3R6PRxsZG+HERvNVqjcVix8fHfX19Nzc3T09PHo+HUjo1NVUulx8fHwFgbW0tEolQSguFwtbWVpU/rlQqs7Ozc3NzqqrmcjmXy9Xe3m61WgcGBubn5wGgo6NjYWEBAEql0t3dHQBUx8ljjNva2orFYnNzM8/zBwcHFoslGo329/cXCgUA6OnpwRh/fHwsLS3lcjm2qm9wQ0NDPB7f398fHBx8eHjIZrOqqhaLRUmSwuFwPB53OBw+ny+dTn9+ftYujed5AEilUhMTE9U9saTX66WUJhKJmv0dHh4Gg0FgF4YxJoQwANNjGIaiKLFYbHp62ul0Li4umqb5H5crSVIymQwEAolEwu12s6SiKNfX15OTkwDgcDguLi4ikUgVUv0zCIJgs9lUVWWlrP3q6qrf72dfAaCrq2tjY0OW5RowTynVNM1qteq6XqW9srJiGAZCSNd1hNDt7W04HGZm+NeFiaKYTCa3t7fHx8fdbjez+9fXV7UR87Cu66Zp1oI1TQsEAl6vN51Os9smhCCEfpbWmMw0TZbBpmm+v7+3traWy2VKKdP825I/M7Isi6IIAFxTU9P6+nomk+nt7X19fX1+fsYY1/ez0+k8Pz+/v7/nMMblcnl4eDifz5+dnWmaVgfGolQq2e32sbGx7wcok8mMjo7C396wVCpFKSWE/ANWXYLwO0+V8wAAAABJRU5ErkJggg=='
        self.download64 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAYAAADgdz34AAAACXBIWXMAAAsSAAALEgHS3X78AAAEl0lEQVRIia2WS2hdZRDHf/Odc+7NvbZpYky1Gk2rEbRa26pt8UWLitYHIqigCD426kIQxCfWhYu68U0RBBERXCq+WrXoorYIFWpbEY0lpk0NTWKtuUlzn+d834yLm5smqQspzurMcM785j+c+eYTTtUcESAzvhIAm/+azHacA4FodszAVNFTrUPm+Q6iecmUyJkEna5OiDCCXCtvJ2cnV+Ep+9R7/VIfQhmeryKeySywok+SSMMKMwqAihDXPIcPDDMURUQCgiPWjJCck6x87ZXXV3cXu3XTO5tkYOvAHbnIfZipTpghLdAMIEngi1cXvtlzwfrHpG0x5ismzsvo0E9D9z7z++M799s2EcSm67OUkAs5cpbzkkoMtPtAAzdXQ9zqjHkt1Ol5SHofx0KWYRUxrdiS3FlLtzz51wd7+v2OQl7qHnPtorUXS3ZxPRUKUT5x4mTDWu559LbCNS+9X9v025Duc4KoYdMAA7A4Mk92EMp/JFIZwR/rx9dL1teVdC2/Qe8yzQg+pS0JvLUzx3hjioPVQamGGlcu47KNq6qrPj+fsd+GeAEYA2SmRQiCNSJKP1Ad3IVaG0nnlWRxKqkkVlYxJxGZwhmFIo34U/fh0Hv4v6YYrY+ihYtkorDUNj+298GPvzv6ZRrkMzA/oyCXh9rEMOOHfiLfcx+5zhXkOnppswxEpJHVxdTjs0CycDHy9XcMlwc5a0E3EoTconOls/dyBsb6lYRLY4m/9T6blDgi8oHw3rPx83fesubl4oVPWFvXBUKoQzqB92Xitpite77n/k/epaN7AZO1CTIROtZ14fJC6ccS9ndGUhRLK0Eum1h2YGpH5eFfD47sjluzcFo+f+vp655F03alNhZhASMjloA1qtzedzab125kiw2QLhHaQ0zIFM2MztUdkBcqx1Lp+0o59NGRP49OVQs0Z3d6nEyMUMP8OGgVtAJaA19CagP4xn4e6DPuPhox1V9HTRFr/h9mRmWkwbJtGSsHK4xXq4cQGQDCDABM0ClEy6DlJiA9DLV90BgktirFzhrPXX0mT6Y9lAaqkAhRItRKGT3bjetTYd2aYM7JYcwm5wwaAP44hDyQYukokg5jliICZoFIoNjZ4Ol1HdhueOPgCLlFjt7twvo63HwztGuipml20lEBBlrGfBXzR5BsDGjOPBrAAkJKRKBwuuepNUXyP5/HN7tKXFGvcuMGY/3qhAO/NLCTJ7kFmIT0OPgjmAhiYKYIASFgGoCUyAILu+o8ckng0jSwsF1YuzxP0hYwm3tizwIIpKPQOIY4BXUYCiiYYWSIKYYHMoRAV1fKTddFxJKQOA/mmW9zFWRjoCmYw6R1lrcg2kxgAfCIeRxKMa+YBSw0Vc7fOScAZuAnMXWYE8yaIUFBDFSbS8sCgscsayZWD3jMAmhT7b8CnDPIeZw6RGTOLmwWFRALMA3BZvkamoBcwM3Zh7MA9Yb5I3v/YKoKTlr9sROKZVrlTGDWsylmkMTGxCQ4h0ObGaT1aRJzHsbtwJJmWSet0/9kIpB69gPbgersJA4oMm/pn6JlQI1/uWX87/YP06p9rkZQnAYAAAAASUVORK5CYII='
        self.github64 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAMAAADXqc3KAAAABGdBTUEAALGPC/xhBQAAAwBQTFRFAAAADAwMDQ0NAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAhyjGAAAAQB0Uk5T////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////AFP3ByUAAAAJcEhZcwAADdUAAA3VAT3WWPEAAAAYdEVYdFNvZnR3YXJlAHBhaW50Lm5ldCA0LjEuMWMqnEsAAABzSURBVChTbYxRFoAgDMPQ+98Z1zbIeJqPbU3RMRfDECqyGpjMg6ivT6NBbKTw5WySq0jKt/sHrXiJ8PwpAAVIgQGkwABSYAApMIAUGEAalFmK9UJ24dC1i7qdj6IO5F+xnxfLu0jS0c7kqxd3Dk+JY8/5AKFrLuM7mfCAAAAAAElFTkSuQmCC'
        self.run64 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAMAAADXqc3KAAAABGdBTUEAALGPC/xhBQAAAwBQTFRFAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAszD0iAAAAQB0Uk5T////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////AFP3ByUAAAAJcEhZcwAADdUAAA3VAT3WWPEAAAAYdEVYdFNvZnR3YXJlAHBhaW50Lm5ldCA0LjEuMWMqnEsAAABqSURBVChTpY5JDsAwCMTy/09TMGvFpVF9aAZPRHpkcXC7OIodPg0uCjPq+MwCrWRGKkiIvLyTqzw3aqoI73eqUNAoXBXlg4zudxF+NONfPIVvbSZPgww5oW0Vz8T4Lgbt/xbjia+rahR5AEYEg4vdzh2JAAAAAElFTkSuQmCC'
        self.storage64 = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAMAAADXqc3KAAAABGdBTUEAALGPC/xhBQAAAwBQTFRFAAAABwcHDQ0NDg4ODw8PFxcXGRkZGhoaGxsbHh4eIyMjJSUlJiYmJycnKCgoMTExMjIyNTU1NjY2Nzc3AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAouNksgAAAQB0Uk5T////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////AFP3ByUAAAAJcEhZcwAADdQAAA3UAe+RuhUAAAAYdEVYdFNvZnR3YXJlAHBhaW50Lm5ldCA0LjEuMWMqnEsAAAC5SURBVChTfZLbDsMgDEPpbb3TDv7/W7PYuAztYUeqhO2QAGowkXIMIeYkaSU4QsNBi4GcyhNINpTglmq4GWSphvy/ldkuLXZ4HmAxy3NmFJaA4guKGCwsjClfV05+fWdhYBtFw+amB292aygW3M7fsPTwjmadZkCvHEtWaAYTViBqVwgTA3tJVnB6D/xhaimItDhjMBvlhtFsaIafnEtOaAY/twAw/eslK70CbX8obUvgJNw9Jv0+Zh8D4s5+VAm/LwAAAABJRU5ErkJggg=='

    def create_menu(self):
        menu_def = [['Photo21 LilDave', ['Help', 'About']],
                    ['File', ['Open', 'Choose Save Directory', 'Exit']],
                    ['Preference', ['Save Preference', 'Load Preference'], ],
                    ['Export', ['---', 'Selected Frame to TSV', 'Selected Traces to TSV',
                                '---', 'Selected Frame to PNG', 'Selected Traces to PNG',
                                '---', 'Selected Regions to TSV',
                                '---', 'Export all of the above',
                                '---', 'Import Regions from TSV(s)']]]
        toolbar_buttons = [[sg.Button('', image_data=self.close64[22:],
                                      button_color=('white', sg.COLOR_SYSTEM_DEFAULT),
                                      pad=(0, 0), key='-close-',
                                      tooltip="Exit"),
                            sg.Button('', image_data=self.timer64[22:],
                                      button_color=('white', sg.COLOR_SYSTEM_DEFAULT),
                                      pad=(0, 0), key='-timer-',
                                      tooltip="Schedule a session at the Little Dave rig (Google Calendar)"),
                            sg.Button('', image_data=self.house64[22:],
                                      button_color=('white', sg.COLOR_SYSTEM_DEFAULT),
                                      pad=(0, 0), key='-house-'),
                            sg.Button('', image_data=self.cpu64[22:],
                                      button_color=('white', sg.COLOR_SYSTEM_DEFAULT),
                                      pad=(0, 0), key='-cpu-'),
                            sg.Button('', image_data=self.download64[22:],
                                      button_color=('white', sg.COLOR_SYSTEM_DEFAULT),
                                      pad=(0, 0), key='-download-'),
                            sg.Button('', image_data=self.github64[22:],
                                      button_color=('white', sg.COLOR_SYSTEM_DEFAULT),
                                      tooltip='Technical Docs',
                                      pad=(0, 0), key='-github-'),
                            sg.Button('', image_data=self.psg64[22:],
                                      button_color=('white', sg.COLOR_SYSTEM_DEFAULT),
                                      pad=(0, 0), key='-psg-',
                                      tooltip="Submit an issue to request a bug fix or new feature."),
                            sg.Button('', image_data=self.run64[22:],
                                      button_color=('white', sg.COLOR_SYSTEM_DEFAULT),
                                      pad=(0, 0), key='-run-'),
                            ]]
        layout = [[sg.Menu(menu_def, )],
                  [sg.Frame('', toolbar_buttons, title_color='white',
                            background_color=sg.COLOR_SYSTEM_DEFAULT, pad=(0, 0))],
                  ]
        return layout

    @staticmethod
    def create_file_browser():
        return [[
            sg.In(size=(25, 1), enable_events=True, key="-FILE-"),
            sg.FileBrowse(key="file_window.browse",
                          # file_types=(("Raw Data Files", "*.zda"))
                          )],
            [sg.Button("Open", key='file_window.open')]]

    @staticmethod
    def create_files_browser(tsv_only=False):
        fb = None
        if tsv_only:
            fb = sg.FilesBrowse(key="file_window.browse",
                                file_types=(("Tab-Separated Value file", "*.tsv"),))
        else:
            fb = sg.FilesBrowse(key="file_window.browse")
        return [
            [sg.In(size=(25, 1), enable_events=True, key="-FILE-"),
             fb],
            [sg.Button("Open", key='file_window.open')]]

    @staticmethod
    def create_folder_browser():
        return [[
            sg.In(size=(25, 1), enable_events=True, key="-FOLDER-"),
            sg.FolderBrowse(key="folder_window.browse")],
            [sg.Button("Open", key='folder_window.open')]]

    @staticmethod
    def create_save_as_browser(file_types):
        return [[
            sg.In(size=(25, 1), enable_events=True, key="-FOLDER-"),
            sg.FileSaveAs(key="save_as_window.browse", file_types=file_types)],
            [sg.Button("Open", key='save_as_window.open')]]

    def create_acquisition_tab(self, gui):
        button_size = (10, 1)
        field_size = (6, 1)
        long_button_size = (15, 1)
        checkbox_size = (8, 1)
        return [
            [sg.Button("STOP!", button_color=('black', 'yellow'), size=button_size, tooltip="Stop Acquisition Task"),
             sg.Button("Take RLI", button_color=('blue', 'white'), size=button_size,
                       tooltip="Record and Compute Resting Light Intensity (RLI) Frame"),
             sg.Button("Reset Cam", button_color=('brown', 'gray'), size=button_size,
                       tooltip="Click this if camera is misbehaving.")],
            [sg.Button("Live Feed", button_color=('black', 'gray'), size=button_size,
                       tooltip='View real-time camera output.'),
             sg.Button("Record", button_color=('black', 'red'), size=button_size,
                       tooltip='Record images while electrically stimulating'),
             sg.Checkbox('Auto RLI', default=self.data.get_is_schedule_rli_enabled(),
                         enable_events=True, key="Auto RLI",
                         size=checkbox_size, tooltip='Automatically take RLI at the beginning of recording.')],
            [sg.HorizontalSeparator()],
            [sg.Text("File Name:", size=(8, 1)),
             sg.InputText(key="File Name",
                          default_text=str(gui.data.db.get_current_filename(no_path=True,
                                                                            extension=self.data.db.extension)),
                          enable_events=False,
                          disabled=True,
                          size=long_button_size,
                          tooltip='Current target file in selected save folder.'),
             sg.Button('<', key='Decrement File', tooltip='Jump to previous existing file.'),
             sg.Button('>', key='Increment File', tooltip='Jump to next existing file.'),
             sg.Checkbox('Average',
                         default=self.data.get_is_trial_averaging_enabled(),
                         enable_events=True,
                         key="Average Trials",
                         tooltip='Compute and display for averages of all trials in this recording set (file).',
                         size=(8, 1)),
             ],
            [sg.Text("Slice:", size=(6, 1), justification='right'),
             sg.InputText(key="Slice Number",
                          default_text=str(gui.data.get_slice_num()),
                          enable_events=True,
                          size=field_size,
                          tooltip='An index for tracking which brain slice to which this data belongs.'),
             sg.Button('<', key='Decrement Slice', tooltip='Decrement slice number.'),
             sg.Button('>', key='Increment Slice', tooltip='Increment slice number.'),
             sg.Text("Location:", size=(8, 1), justification='right',
                     tooltip='An index for tracking which electrode location placement to which this data belongs.'),
             sg.InputText(key="Location Number",
                          default_text=str(gui.data.get_location_num()),
                          enable_events=True,
                          size=field_size),
             sg.Button('<', key='Decrement Location', tooltip='Decrement location number.'),
             sg.Button('>', key='Increment Location', tooltip='Inccrement location number.')],
            [sg.Text("Record:", size=(6, 1), justification='right'),
             sg.InputText(key="Record Number",
                          default_text=str(gui.data.get_record_num()),
                          enable_events=True,
                          size=field_size,
                          tooltip='An index for tracking which recording (trial set) to which this data belongs.'),
             sg.Button('<', key='Decrement Record', tooltip="Decrement record number."),
             sg.Button('>', key='Increment Record', tooltip="Increment record number."),
             sg.Text("Trial:", size=(8, 1), justification='right'),
             sg.InputText(key="Trial Number",
                          default_text=str(gui.data.get_current_trial_index()),
                          enable_events=True,
                          size=field_size,
                          tooltip="An index for tracking trial number. 'None' indicates all-trial averaging."),
             sg.Button('<', key='Decrement Trial', tooltip="Increment trial number."),
             sg.Button('>', key='Increment Trial', tooltip="Decrement trial number.")],
        ]

    def create_roi_tab(self, gui):
        button_size = (6, 1)
        long_button_size = (17, 1)
        t_pre_stim = gui.roi.get_time_window('pre_stim')
        t_stim = gui.roi.get_time_window('stim')
        if t_pre_stim[1] == -1:
            t_pre_stim[1] = self.data.get_num_pts()
        if t_stim[1] == -1:
            t_stim[1] = self.data.get_num_pts()
        int_pts = self.data.get_int_pts()
        return [
            [sg.Button("Pre-Stim Window",
                       button_color=('black', 'orange'),
                       size=long_button_size,
                       tooltip="A time window indicating pre-stimulation "
                               "data to use as a control for ROI Identification."),
             sg.InputText(key="Time Window Start frames pre_stim",
                          default_text=str(t_pre_stim[0]),
                          enable_events=True,
                          size=button_size,
                          tooltip="A time window indicating pre-stimulation "
                                  "data to use as a control for ROI Identification."),
             sg.Text(" to "),
             sg.InputText(key="Time Window End frames pre_stim",
                          default_text=str(t_pre_stim[1]),
                          enable_events=True,
                          size=button_size,
                          tooltip="A time window indicating pre-stimulation "
                                  "data to use as a control for ROI Identification."),
             sg.Text(" frames")],
            [sg.Text("", size=long_button_size),
             sg.InputText(key="Time Window Start (ms) pre_stim",
                          default_text=str(t_pre_stim[0] * int_pts),
                          enable_events=True,
                          size=button_size,
                          tooltip="A time window indicating pre-stimulation "
                                  "data to use as a control for ROI Identification."),
             sg.Text(" to "),
             sg.InputText(key="Time Window End (ms) pre_stim",
                          default_text=str(t_pre_stim[1] * int_pts),
                          enable_events=True,
                          size=button_size,
                          tooltip="A time window indicating pre-stimulation "
                                  "data to use as a control for ROI Identification."),
             sg.Text(" ms")],
            [sg.Button("Stim Window",
                       button_color=('black', 'orange'),
                       size=long_button_size,
                       tooltip="A time window indicating the stimulation and stimulation response period."),
             sg.InputText(key="Time Window Start frames stim",
                          default_text=str(t_stim[0]),
                          enable_events=True,
                          size=button_size,
                          tooltip="A time window indicating the stimulation and stimulation response period."),
             sg.Text(" to "),
             sg.InputText(key="Time Window End frames stim",
                          default_text=str(t_stim[1]),
                          enable_events=True,
                          size=button_size,
                          tooltip="A time window indicating the stimulation and stimulation response period."),
             sg.Text(" frames")],
            [sg.Text("", size=long_button_size),
             sg.InputText(key="Time Window Start (ms) stim",
                          default_text=str(t_stim[0] * int_pts),
                          enable_events=True,
                          size=button_size,
                          tooltip="A time window indicating the stimulation and stimulation response period."),
             sg.Text(" to "),
             sg.InputText(key="Time Window End (ms) stim",
                          default_text=str(t_stim[1] * int_pts),
                          enable_events=True,
                          size=button_size,
                          tooltip="A time window indicating the stimulation and stimulation response period."),
             sg.Text(" ms")],
            [sg.Button("ROI Identifier Config",
                       button_color=('black', 'green'),
                       size=long_button_size),
             sg.Checkbox('Identify ROI',
                         default=self.data.meta.is_roi_enabled,
                         enable_events=True,
                         key="Identify ROI",
                         size=long_button_size)],
        ]

    def create_hyperslicer_tab(self):
        long_button_size = (17, 1)
        return [[sg.Button("Launch Hyperslicer",
                           button_color=('white', 'blue'),
                           size=long_button_size)]]

    def create_freq_decomp_tab(self):
        return []

    def create_analyses_tab_group(self, gui):
        return [sg.TabGroup([[
            sg.Tab('Cluster/ROI', self.create_roi_tab(gui)),
            sg.Tab("Hyperslicer", self.create_hyperslicer_tab()),
            sg.Tab("FreqDecomp", self.create_freq_decomp_tab()),
        ]])]

    def create_analysis_tab(self, gui):
        return [
                   [sg.Checkbox('Analysis Mode',
                                default=self.data.get_is_analysis_only_mode_enabled(),
                                enable_events=True,
                                key="Analysis Mode",
                                size=(12, 1), tooltip='Automatically save .npy/.pbz2 between records (sets of trials)'),
                    sg.Button("Save Analysis", button_color=('white', 'green'), size=(12, 1),
                              tooltip='Export all analyzed data to Origin/Excel/R-'
                                      'interoperable formats (tab-separated values)'),
                    sg.Button("Save", button_color=('white', 'green'), size=(10, 1),
                              tooltip='Save images as .npy file and metadata/preferences as .pbz2 file')],
               ] + [self.create_analyses_tab_group(gui)]

    def create_array_tab(self, gui):
        checkbox_size = (10, 1)
        background_options = gui.data.get_background_options()
        colormap_options = gui.fv.get_color_map_options()
        return [
            [sg.Text("Pixel Value:", size=checkbox_size),
             sg.Combo(background_options,
                      enable_events=True,
                      default_value=background_options[gui.data.get_background_option_index()],
                      key="Select Background",
                      disabled=gui.fv.get_show_rli_flag(),
                      tooltip="The data to compute, export, or display in the Frame Viewer.")],
            [sg.Text("Colormap:", size=checkbox_size),
             sg.Combo(colormap_options,
                      enable_events=True,
                      default_value=gui.fv.get_color_map_option_name(),
                      key="Select Colormap",
                      tooltip="Colormap scheme for intensity images shown in the Frame Viewer.")],
            [sg.Checkbox('Show RLI',
                         default=gui.fv.get_show_rli_flag(),
                         enable_events=True,
                         key="Show RLI",
                         size=checkbox_size,
                         tooltip="When selected, RLI frame is shown in the Frame Viewer.")],
            [sg.Checkbox('RLI Division',
                         default=self.data.get_is_rli_division_enabled(),
                         enable_events=True,
                         key="RLI Division",
                         size=checkbox_size)],
            [sg.Checkbox('Data Inverse',
                         default=self.data.get_is_data_inverse_enabled(),
                         enable_events=True,
                         key='Data Inverse',
                         size=checkbox_size)],
            [sg.Text("Digital Binning:"), sg.InputText(default_text=gui.data.meta.binning,
                                                       key="Digital Binning",
                                                       size=(5, 1),
                                                       enable_events=True)],
            [sg.Button("Load Image", button_color=('gray', 'black'))],
        ]

    def create_baseline_tab(self, gui):
        button_size = (8, 1)
        double_button_size = (20, 1)
        field_text_size = (12, 1)
        baseline_correction_options = self.data.core.get_baseline_correction_options()
        baseline_skip_default = self.data.core.get_baseline_skip_window()
        button_size = (6, 1)
        long_button_size = (17, 1)
        t_window = gui.data.get_measure_window()
        if t_window[1] == -1:
            t_window[1] = self.data.get_num_pts()
        int_pts = self.data.get_int_pts()
        return [
            [sg.Text('Baseline Correction:', size=(16, 1)),
             sg.Combo(baseline_correction_options,
                      enable_events=True,
                      default_value=baseline_correction_options[self.data.core.get_baseline_correction_type_index()],
                      key="Select Baseline Correction")],
            [sg.Button("Baseline Skip Window",
                       button_color=('black', 'orange'),
                       size=double_button_size)],
            [sg.InputText(key="Baseline Skip Window Start frames",
                          default_text=str(baseline_skip_default[0]),
                          enable_events=True,
                          size=button_size),
             sg.Text(" to "),
             sg.InputText(key="Baseline Skip Window End frames",
                          default_text=str(baseline_skip_default[1]),
                          enable_events=True,
                          size=button_size),
             sg.Text(" frames")],
            [sg.InputText(key="Baseline Skip Window Start (ms)",
                          default_text=str(baseline_skip_default[0] * int_pts)[:6],
                          enable_events=True,
                          size=button_size),
             sg.Text(" to "),
             sg.InputText(key="Baseline Skip Window End (ms)",
                          default_text=str(baseline_skip_default[1] * int_pts)[:6],
                          enable_events=True,
                          size=button_size),
             sg.Text(" ms")],
            [sg.Button("Measure Window",
                       button_color=('black', 'orange'),
                       size=long_button_size,
                       tooltip="A time window to which to restrict processing and analysis.")],
            [sg.InputText(key="Measure Window Start frames",
                          default_text=str(t_window[0])[:6],
                          enable_events=True,
                          size=button_size,
                          tooltip="A time window to which to restrict processing and analysis."),
             sg.Text(" to "),
             sg.InputText(key="Measure Window End frames",
                          default_text=str(t_window[1])[:6],
                          enable_events=True,
                          size=button_size,
                          tooltip="A time window to which to restrict processing and analysis."),
             sg.Text(" frames")],
            [sg.InputText(key="Measure Window Start (ms)",
                          default_text=str(t_window[0] * int_pts)[:6],
                          enable_events=True,
                          size=button_size,
                          tooltip="A time window to which to restrict processing and analysis."),
             sg.Text(" to "),
             sg.InputText(key="Measure Window End (ms)",
                          default_text=str(t_window[1] * int_pts)[:6],
                          enable_events=True,
                          size=button_size,
                          tooltip="A time window to which to restrict processing and analysis."),
             sg.Text(" ms")]
        ]

    def create_artifact_tab(self, gui):
        button_size = (10, 1)
        long_button_size = (14, 1)

        t_cae = gui.data.get_artifact_exclusion_window()
        if t_cae[1] == -1:
            t_cae[1] = self.data.get_num_pts()
        int_pts = self.data.get_int_pts()
        return [
            [sg.Button("Exclusion Window",
                       button_color=('black', 'orange'),
                       size=long_button_size,
                       tooltip="Time window to be adjusted to an interval that excludes camera artifacts."),
             sg.InputText(key="Camera Artifact Exclusion Window Start frames",
                          default_text=str(t_cae[0]),
                          enable_events=True,
                          size=button_size,
                          tooltip="Time window to be adjusted to an interval that excludes camera artifacts."),
             sg.Text(" to "),
             sg.InputText(key="Camera Artifact Exclusion Window End frames",
                          default_text=str(t_cae[1]),
                          enable_events=True,
                          size=button_size,
                          tooltip="Time window to be adjusted to an interval that excludes camera artifacts."),
             sg.Text(" frames")],
            [sg.Text("", size=long_button_size),
             sg.InputText(key="Camera Artifact Exclusion Window Start (ms)",
                          default_text=str(t_cae[0] * int_pts),
                          enable_events=True,
                          size=button_size,
                          tooltip="Time window to be adjusted to an interval that excludes camera artifacts."),
             sg.Text(" to "),
             sg.InputText(key="Camera Artifact Exclusion Window End (ms)",
                          default_text=str(t_cae[1] * int_pts),
                          enable_events=True,
                          size=button_size,
                          tooltip="Time window to be adjusted to an interval that excludes camera artifacts."),
             sg.Text(" ms")],
            [sg.Text("", size=button_size, justification='right')]
        ]

    def create_contrast_tab(self):
        button_size = (10, 1)
        slider_size = (20, 40)
        return [
            [sg.Text('Contrast Adjuster',
             tooltip="This contrast adjustment induces saturation. It"
                     " applies to visualization only, not to exported data.")],
            [sg.Slider(range=(0.5, 50.0),
                       default_value=self.data.get_contrast_scaling(),
                       resolution=1.0,
                       enable_events=True,
                       size=slider_size,
                       orientation='horizontal',
                       tooltip="This contrast adjustment induces saturation. It"
                               " applies to visualization only, not to exported data.",
                       key="Contrast Scaling")]]

    def create_filter_tab(self):
        button_size = (10, 1)
        slider_size = (20, 40)
        t_filter_options = self.data.core.get_temporal_filter_options()
        return [
            [sg.Checkbox('T-Filter',
                         default=self.data.core.get_is_temporal_filter_enabled(),
                         enable_events=True,
                         key='T-Filter',
                         size=button_size),
             sg.Combo(t_filter_options,
                      enable_events=True,
                      default_value=t_filter_options[self.data.core.get_temporal_filter_index()],
                      key="Select Temporal Filter")],
            [sg.Text("Radius (pt):", size=button_size, justification='right'),
             sg.Slider(range=(0.5, 50.0),
                       default_value=self.data.core.get_temporal_filter_radius(),
                       resolution=1.0,
                       enable_events=True,
                       size=slider_size,
                       orientation='horizontal',
                       key="Temporal Filter Radius")],
            [sg.Text('')],
            [sg.Checkbox('S-Filter',
                         default=self.data.core.get_is_spatial_filter_enabled(),
                         enable_events=True,
                         key='S-Filter',
                         size=button_size)],
            [sg.Text("Sigma (px):", size=button_size, justification='right'),
             sg.Slider(range=(0.1, 2.0),
                       default_value=self.data.core.get_spatial_filter_sigma(),
                       resolution=.1,
                       enable_events=True,
                       size=slider_size,
                       orientation='horizontal',
                       key="Spatial Filter Sigma")]
        ]

    def create_left_column(self, gui):
        acquisition_tab_layout = self.create_acquisition_tab(gui)
        analysis_tab_layout = self.create_analysis_tab(gui)
        array_tab_layout = self.create_array_tab(gui)
        filter_tab_layout = self.create_filter_tab()
        baseline_tab_layout = self.create_baseline_tab(gui)
        contrast_layout = self.create_contrast_tab()

        tab_group_basic = [sg.TabGroup([[
            sg.Tab('Acquisition', acquisition_tab_layout),
            sg.Tab('Analysis', analysis_tab_layout),
        ]])]

        tab_group_advanced = [sg.TabGroup([[
            sg.Tab('Array', array_tab_layout),
            sg.Tab("Baseline", baseline_tab_layout),
            sg.Tab("Filter", filter_tab_layout),
            sg.Tab("Contrast", contrast_layout),
        ]])]

        frame_viewer_layout = [
            [sg.Column(
                layout=[
                    [sg.Canvas(key='frame_canvas',
                               size=self.plot_size,
                               tooltip="Frame Viewer"
                               )]
                ],
                background_color='#DAE0E6',
                pad=(0, 0))]]

        return frame_viewer_layout + \
               [tab_group_basic + tab_group_advanced]

    def create_acqui_controls_tab(self):
        cell_size = (10, 1)
        return [[sg.Text('', size=cell_size),
                 sg.Text('ONSET', size=cell_size),
                 sg.Text('DURATION', size=cell_size)],
                [sg.Text("Acquisition", size=cell_size),
                 sg.InputText(key="Acquisition Onset",
                              default_text=str(self.data.get_acqui_onset()),
                              enable_events=True,
                              size=cell_size,
                              tooltip='Time at which image recording begins, in milliseconds.'),
                 sg.InputText(key="Acquisition Duration",
                              default_text=str(self.data.get_acqui_duration()),
                              enable_events=True,
                              size=cell_size,
                              tooltip='Length of time of image recording, in milliseconds.'),
                 sg.Text(" ms", size=cell_size)],
                [sg.Text("Stimulator #1", size=cell_size),
                 sg.InputText(key="Stimulator #1 Onset",
                              default_text=str(self.data.get_stim_onset(1)),
                              enable_events=True,
                              size=cell_size,
                              tooltip='Time at which electrode #1 stimulation begins, in milliseconds.'),
                 sg.InputText(key="Stimulator #1 Duration",
                              default_text=str(self.data.get_stim_duration(1)),
                              enable_events=True,
                              size=cell_size,
                              tooltip='Length of time of electrode #1 stimulation, in milliseconds.'),
                 sg.Text(" ms", size=cell_size)],
                [sg.Text("Stimulator #2", size=cell_size),
                 sg.InputText(key="Stimulator #2 Onset",
                              default_text=str(self.data.get_stim_onset(2)),
                              enable_events=True,
                              size=cell_size,
                              tooltip='Time at which electrode #2 stimulation begins, in milliseconds.'),
                 sg.InputText(key="Stimulator #2 Duration",
                              default_text=str(self.data.get_stim_duration(2)),
                              enable_events=True,
                              size=cell_size,
                              tooltip='Length of time of electrode #1 stimulation, in milliseconds.'),
                 sg.Text(" ms", size=cell_size)]]

    def create_ttl_output_tab(self):
        camera_programs = self.data.display_camera_programs
        cell_size = (10, 1)
        double_cell_size = (20, 1)
        return [[sg.Text("Number of Points:", size=double_cell_size),
                 sg.InputText(key="Number of Points",
                              default_text=str(self.data.get_num_pts()),
                              enable_events=True,
                              size=cell_size,
                              tooltip="Number of frames to acquire at this camera frequency.")],
                [sg.Text("Camera Program:", size=double_cell_size,
                         tooltip="A pre-programmed camera setting that determines images resolution"
                                 " and sampling frequency."),
                 sg.Combo(camera_programs,
                          enable_events=True,
                          default_value=camera_programs[self.data.get_camera_program()],
                          key="-CAMERA PROGRAM-",
                          tooltip="A pre-programmed camera setting that determines images resolution"
                                  " and sampling frequency.")]]

    def create_pulses_tab(self):
        cell_size = (10, 1)
        double_cell_size = (20, 1)
        return [[sg.Text("", size=double_cell_size),
                 sg.Text("Stimulator #1", size=cell_size),
                 sg.Text("Stimulator #2", size=cell_size)],
                [sg.Text("Number of pulses:", size=double_cell_size),
                 sg.InputText(key="num_pulses Stim #1",
                              default_text=str(self.data.hardware.get_num_pulses(channel=1)),
                              enable_events=True,
                              size=cell_size,
                              tooltip="Number of pulses in each burst from electrode #1."),
                 sg.InputText(key="num_pulses Stim #2",
                              default_text=str(self.data.hardware.get_num_pulses(channel=2)),
                              enable_events=True,
                              size=cell_size,
                              tooltip="Number of pulses in each burst from electrode #2.")],
                [sg.Text("Interval between pulses:", size=double_cell_size),
                 sg.InputText(key="int_pulses Stim #1",
                              default_text=str(self.data.hardware.get_int_pulses(channel=1)),
                              enable_events=True,
                              size=cell_size,
                              tooltip="Interval between pulses in each burst from electrode #1."),
                 sg.InputText(key="int_pulses Stim #2",
                              default_text=str(self.data.hardware.get_int_pulses(channel=2)),
                              enable_events=True,
                              size=cell_size,
                              tooltip="Interval between pulses in each burst from electrode #2."),
                 sg.Text(" ms", size=cell_size)],
                [sg.Text("Number of bursts:", size=double_cell_size),
                 sg.InputText(key="num_bursts Stim #1",
                              default_text=str(self.data.hardware.get_num_bursts(channel=1)),
                              enable_events=True,
                              size=cell_size,
                              tooltip="Number of bursts (sets of pulses) from electrode #1."),
                 sg.InputText(key="num_bursts Stim #2",
                              default_text=str(self.data.hardware.get_num_bursts(channel=2)),
                              enable_events=True,
                              size=cell_size,
                              tooltip="Number of bursts (sets of pulses) from electrode #2.")],
                [sg.Text("Interval between bursts:", size=double_cell_size),
                 sg.InputText(key="int_bursts Stim #1",
                              default_text=str(self.data.hardware.get_int_bursts(channel=1)),
                              enable_events=True,
                              size=cell_size,
                              tooltip="Interval between bursts (sets of pulses) from electrode #1."),
                 sg.InputText(key="int_bursts Stim #2",
                              default_text=str(self.data.hardware.get_int_bursts(channel=2)),
                              enable_events=True,
                              size=cell_size,
                              tooltip="Interval between bursts (sets of pulses) from electrode #2."),
                 sg.Text(" ms", size=cell_size)]]

    def create_trials_tab(self):
        cell_size = (10, 1)
        double_cell_size = (20, 1)

        return [[sg.Text("Number of Trials:", size=double_cell_size),
                 sg.InputText(key="num_trials",
                              default_text=str(self.data.get_num_trials()),
                              enable_events=True,
                              size=cell_size,
                              tooltip='Number of trials in each recording.'),
                 sg.Text("", size=cell_size)],
                [sg.Text("Interval between Trials:", size=double_cell_size),
                 sg.InputText(key="int_trials",
                              default_text=str(self.data.get_int_trials()),
                              enable_events=True,
                              size=cell_size,
                              tooltip='Number of seconds between trials in each recording.'),
                 sg.Text(" s", size=cell_size)]]

    def create_records_tab(self):
        cell_size = (10, 1)
        double_cell_size = (20, 1)
        return [[sg.Text("", size=double_cell_size)],
                [sg.Text("Record (Sets of Trials) Controls")],
                [sg.Text("Number of Recordings:", size=double_cell_size),
                 sg.InputText(key="num_records",
                              default_text=str(self.data.get_num_records()),
                              enable_events=True,
                              size=cell_size,
                              tooltip='Number of recordings (sets of trials).'),
                 sg.Text("", size=cell_size)],
                [sg.Text("Interval between Record:", size=double_cell_size),
                 sg.InputText(key="int_records",
                              default_text=str(self.data.get_int_records()),
                              enable_events=True,
                              size=cell_size,
                              tooltip='Number of seconds between recordings (sets of trials).'),
                 sg.Text(" s", size=cell_size)]]

    def create_daq_config_tab(self):
        daq_timeline_canvas = [[sg.Canvas(key='daq_canvas', size=self.plot_size)]]
        daq_config_tab_group = [
            [sg.TabGroup([[
                sg.Tab('Onset/Duration', self.create_acqui_controls_tab() + self.create_ttl_output_tab()),
                sg.Tab('Pulses', self.create_pulses_tab()),
                sg.Tab('Trials', self.create_trials_tab() + self.create_records_tab()),
            ]])]]
        return daq_timeline_canvas + daq_config_tab_group

    @staticmethod
    def create_display_tab(gui):
        button_size = (12, 1)
        display_value_options = gui.tv.get_display_value_options()
        return [
            [sg.Text("Value:", size=button_size),
             sg.Combo(display_value_options,
                      enable_events=True,
                      default_value=display_value_options[gui.get_display_value_option_index()],
                      key="Select Display Value",
                      tooltip='Value to display in Trace Viewer next to each applicable trace selection.')]
        ]

    def create_simulation_tab(self):
        return []

    def create_trace_viewer_tab(self, gui):
        trace_viewer_canvas = [
            [sg.Canvas(key='trace_canvas', size=self.plot_size,
                       tooltip='Trace Viewer')]
        ]
        trace_viewer_tab_group = [
            [sg.TabGroup([[
                sg.Tab('Display', self.create_display_tab(gui)),
                sg.Tab('Artifact', self.create_artifact_tab(gui)),
                sg.Tab('Simulation', self.create_simulation_tab())]])
            ]]
        return trace_viewer_canvas + trace_viewer_tab_group

    def create_notepad_tab(self):
        return [[sg.Multiline(key="Notepad",
                              default_text=self.data.meta.notepad_text,
                              enable_events=True,
                              size=(50, 600),
                              tooltip="Notes for this recording, to be saved to metadata file.")]]

    def create_time_course_tab(self, gui):
        return [
            [sg.Canvas(key='time_course_canvas', size=self.plot_size,
                       tooltip='Time Course Viewer')],
            [sg.Text("Available Records:")],
            [sg.Listbox(values=gui.data.get_data_filenames_in_folder(),
                        size=(50, 20),
                        tooltip="Hold CTRL or SHIFT to select ranges of records"
                                " for which to plot aggregated values.",
                        key="Time Course File Selector",
                        enable_events=True,
                        select_mode=sg.LISTBOX_SELECT_MODE_EXTENDED)]
        ]

    def create_right_column(self, gui):

        tab_group_right = [sg.TabGroup([[
            sg.Tab('Trace Viewer', self.create_trace_viewer_tab(gui)),
            sg.Tab('DAQ Config', self.create_daq_config_tab()),  # plotting a small timeline of record/stim events
            sg.Tab('Notepad', self.create_notepad_tab(), key='Notes'),
            sg.Tab('Time Course', self.create_time_course_tab(gui), key='Time Course')
        ]])]
        return [tab_group_right]

    @staticmethod
    def list_hardware_settings():
        return ['Number of Points',
                'Acquisition Onset',
                "Acquisition Duration",
                "Stimulator #1 Onset",
                "Stimulator #2 Onset",
                "Stimulator #1 Duration",
                "Stimulator #2 Duration",
                "-CAMERA PROGRAM-",
                "num_pulses Stim #1",
                "num_pulses Stim #2",
                "int_pulses Stim #1",
                "int_pulses Stim #2",
                "num_bursts Stim #1",
                "num_bursts Stim #2",
                "int_bursts Stim #1",
                "int_bursts Stim #2",
                "num_trials",
                "int_trials",
                "Auto RLI",
                "Analysis Mode"
                ]

    @staticmethod
    def list_file_navigation_fields():
        return ["Increment Trial",
                "Decrement Trial",
                "Increment Record",
                "Decrement Record",
                "Increment Location",
                "Decrement Location",
                "Increment Slice",
                "Decrement Slice",
                "Trial Number",
                "Location Number",
                "Record Number",
                "Slice Number",
                'num_records',
                'int_records']

    @staticmethod
    def list_hardware_events():
        return ["Live Feed", "Take RLI", "Record", "Reset Cam"]

    @staticmethod
    def list_file_events():
        return ["Save Analysis", "Save"]

    @staticmethod
    def create_roi_settings_form(gui):
        cell_size = (10, 1)
        double_cell_size = (20, 1)
        return [
            [sg.Text("", size=double_cell_size),
             sg.Text("Raw Value", size=cell_size),
             sg.Text("Percentile", size=cell_size), ],
            [sg.Text("Pixel-wise SNR cutoff", size=double_cell_size),
             sg.InputText(key="Pixel-wise SNR cutoff Raw",
                          default_text=str(gui.roi.get_cutoff("pixel", "raw")),
                          enable_events=True,
                          size=cell_size,
                          tooltip='SNR value below which to exclude pixels from ROI Identification.'),
             sg.InputText(key="Pixel-wise SNR cutoff Percentile",
                          default_text=str(gui.roi.get_cutoff("pixel", "percentile")),
                          enable_events=True,
                          size=cell_size,
                          tooltip='SNR percentile below which to exclude pixels from ROI Identification.')],
            [sg.Text("Cluster-wise SNR cutoff", size=double_cell_size),
             sg.InputText(key="Cluster-wise SNR cutoff Raw",
                          default_text=str(gui.roi.get_cutoff("cluster", "raw")),
                          enable_events=True,
                          size=cell_size,
                          tooltip='SNR value below which to exclude clusters from ROI Identification.'),
             sg.InputText(key="Cluster-wise SNR cutoff Percentile",
                          default_text=str(gui.roi.get_cutoff("cluster", "percentile")),
                          enable_events=True,
                          size=cell_size,
                          tooltip='SNR percentile below which to exclude clusters from ROI Identification.')],
            [sg.Text("ROI-wise SNR cutoff", size=double_cell_size),
             sg.InputText(key="ROI-wise SNR cutoff Raw",
                          default_text=str(gui.roi.get_cutoff("roi_snr", "raw")),
                          enable_events=True,
                          size=cell_size,
                          tooltip='SNR value below which to exclude ROIs from ROI Identification.'),
             sg.InputText(key="ROI-wise SNR cutoff Percentile",
                          default_text=str(gui.roi.get_cutoff("roi_snr", "percentile")),
                          enable_events=True,
                          size=cell_size,
                          tooltip='SNR percentile below which to exclude ROIs from ROI Identification.')],
            [sg.Text("ROI-wise Amplitude cutoff", size=double_cell_size),
             sg.InputText(key="ROI-wise Amplitude cutoff Raw",
                          default_text=str(gui.roi.get_cutoff("roi_amplitude", "raw")),
                          enable_events=True,
                          size=cell_size,
                          tooltip='MaxAmp value below which to exclude ROIs from ROI Identification.'),
             sg.InputText(key="ROI-wise Amplitude cutoff Percentile",
                          default_text=str(gui.roi.get_cutoff("roi_amplitude", "percentile")),
                          enable_events=True,
                          size=cell_size,
                          tooltip='MaxAmp percentile below which to exclude ROIs from ROI Identification.')],
            [sg.Text("Number of Clusters", size=double_cell_size),
             sg.InputText(key="roi.k_clusters",
                          default_text=str(gui.roi.get_k_clusters()),
                          enable_events=True,
                          size=cell_size),
             sg.Text('', size=cell_size)],
            [sg.Button("View Silhouette Plot", button_color=('gray', 'black'), size=double_cell_size)],
            [sg.Button("View Elbow Plot", button_color=('gray', 'black'), size=double_cell_size)],
            [sg.Button("Load ROI Data from File", button_color=('gray', 'black'), size=double_cell_size)],
            [sg.Button("Save ROI Data to File", button_color=('gray', 'black'), size=double_cell_size)],
            [sg.Button("OK", key='Exit ROI')]
        ]
