'''
@Author: Pawn
@Date: 2020-12-26
@LastEditTime: 2020-12-26 14:46:08
@FilePath: systeam.py
'''


def replSetEnable(flag=0):
    import uos
    import misc
    import ujson
    
    if "system_config.json" in uos.listdir("usr/"):
        try:
            with open("/usr/system_config.json", "w+", encoding='utf-8') as fd:
                repl_data = ujson.dumps({"replFlag": flag})
                fd.write(repl_data)
        except:
            raise OSError("The config.JSON file is abnormal, please check!")
    misc.replEnable(flag)
    return 0