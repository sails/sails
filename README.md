sails
=====

c++ base library for linux/osx server side development

支持两种编译方式，make和cmake；
一般用make即可,但对于要编译到ios平台上来说，用cmake更方便,先创建一个build文件夹，然后执行cmake -GXcode ../;然后就可以用xcode打开了，不过要改变一下它的目标平台
